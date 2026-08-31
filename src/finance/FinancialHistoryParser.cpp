#include "FinancialHistoryParser.hpp"

#include <charconv>
#include <stdexcept>
#include <string>

namespace
{
constexpr auto CheckoDataField = "data";
constexpr auto CheckoCurrentAmount = "СумОтч";
constexpr auto CheckoTotalAmount = "Итог";

constexpr auto DaDataSuggestions = "suggestions";
constexpr auto DaDataData = "data";
constexpr auto DaDataFinanceHistory = "finance_history";
constexpr auto DaDataMetrics = "metrics";
constexpr auto DaDataYear = "year";
constexpr auto DaDataCode = "code";
constexpr auto DaDataValue = "value";

void AssertIsResponseValid(const nlohmann::json& response)
{
	if (!response.is_object())
	{
		throw std::invalid_argument("Ответ с отчетностью должен быть объектом");
	}
}

std::optional<int> ToInteger(const std::string& text)
{
	int value = 0;
	const char* const begin = text.data();
	const char* const end = text.data() + text.size();

	const auto [stopped, error] = std::from_chars(begin, end, value);
	if (error != std::errc{} || stopped != end)
	{
		return std::nullopt;
	}

	return value;
}

std::optional<double> ReadCheckoLine(const nlohmann::json& node)
{
	if (node.is_number())
	{
		return node.get<double>();
	}

	if (!node.is_object())
	{
		return std::nullopt;
	}

	for (const auto* field : {CheckoCurrentAmount, CheckoTotalAmount})
	{
		if (node.contains(field) && node.at(field).is_number())
		{
			return node.at(field).get<double>();
		}
	}

	return std::nullopt;
}

FinancialYear ParseCheckoYear(const int year, const nlohmann::json& node)
{
	FinancialYear report;
	report.year = year;

	for (const auto& [key, value] : node.items())
	{
		const std::optional<int> code = ToInteger(key);
		if (!code.has_value())
		{
			continue;
		}

		const std::optional<double> amount = ReadCheckoLine(value);
		if (!amount.has_value())
		{
			continue;
		}

		report.lines[code.value()] = amount.value();
	}

	return report;
}

const nlohmann::json& FindDaDataParty(const nlohmann::json& response)
{
	static const nlohmann::json empty = nlohmann::json::object();

	if (!response.contains(DaDataSuggestions) || !response.at(DaDataSuggestions).is_array())
	{
		return empty;
	}

	const nlohmann::json& suggestions = response.at(DaDataSuggestions);
	if (suggestions.empty() || !suggestions.front().contains(DaDataData))
	{
		return empty;
	}

	return suggestions.front().at(DaDataData);
}

FinancialYear ParseDaDataYear(const nlohmann::json& node)
{
	FinancialYear report;

	if (node.contains(DaDataYear) && node.at(DaDataYear).is_number())
	{
		report.year = node.at(DaDataYear).get<int>();
	}

	if (!node.contains(DaDataMetrics) || !node.at(DaDataMetrics).is_array())
	{
		return report;
	}

	for (const auto& metric : node.at(DaDataMetrics))
	{
		if (!metric.contains(DaDataCode) || !metric.contains(DaDataValue))
		{
			continue;
		}

		const std::optional<int> code = ToInteger(metric.at(DaDataCode).get<std::string>());
		if (!code.has_value() || !metric.at(DaDataValue).is_number())
		{
			continue;
		}

		report.lines[code.value()] = metric.at(DaDataValue).get<double>();
	}

	return report;
}
} // namespace

FinancialHistory FinancialHistoryParser::ParseChecko(const nlohmann::json& response)
{
	AssertIsResponseValid(response);

	FinancialHistory history;
	if (!response.contains(CheckoDataField) || !response.at(CheckoDataField).is_object())
	{
		return history;
	}

	for (const auto& [key, node] : response.at(CheckoDataField).items())
	{
		const std::optional<int> year = ToInteger(key);
		if (!year.has_value() || !node.is_object())
		{
			continue;
		}

		history.AddYear(ParseCheckoYear(year.value(), node));
	}

	return history;
}

FinancialHistory FinancialHistoryParser::ParseDaData(const nlohmann::json& response)
{
	AssertIsResponseValid(response);

	FinancialHistory history;
	const nlohmann::json& party = FindDaDataParty(response);

	if (!party.contains(DaDataFinanceHistory) || !party.at(DaDataFinanceHistory).is_array())
	{
		return history;
	}

	for (const auto& node : party.at(DaDataFinanceHistory))
	{
		FinancialYear report = ParseDaDataYear(node);
		if (report.year == 0 || report.lines.empty())
		{
			continue;
		}

		history.AddYear(std::move(report));
	}

	return history;
}

FinancialHistory FinancialHistoryParser::Merge(
	const FinancialHistory& primary,
	const FinancialHistory& secondary)
{
	FinancialHistory merged;

	for (const int year : primary.GetYears())
	{
		merged.AddYear(primary.GetYear(year));
	}

	for (const int year : secondary.GetYears())
	{
		merged.MergeYear(secondary.GetYear(year));
	}

	return merged;
}