#include "MetricReport.hpp"

#include "MetricIndex.hpp"

namespace
{
std::vector<MetricRow> BuildRows(const CompanyAnalytics& analytics)
{
	CompanyAnalytics snapshot = analytics;
	const MetricIndex index(snapshot);

	std::vector<MetricRow> rows;
	rows.reserve(index.ListAll().size());

	for (const auto& [id, title, unit, group, value] : index.ListAll())
	{
		rows.push_back({id,
			title,
			unit,
			group,
			*value});
	}

	return rows;
}
} // namespace

MetricReport::MetricReport(const CompanyAnalytics& analytics)
	: m_rows(BuildRows(analytics))
{
}

const std::vector<MetricRow>& MetricReport::GetRows() const noexcept
{
	return m_rows;
}

std::vector<MetricRow> MetricReport::GetGroup(const MetricGroup group) const
{
	std::vector<MetricRow> rows;

	for (const auto& row : m_rows)
	{
		if (row.group == group)
		{
			rows.push_back(row);
		}
	}

	return rows;
}

std::vector<MetricGroup> MetricReport::ListGroups()
{
	return {MetricGroup::Revenue,
		MetricGroup::Profit,
		MetricGroup::Margins,
		MetricGroup::CashFlow,
		MetricGroup::WorkingCapital,
		MetricGroup::Debt,
		MetricGroup::Balance,
		MetricGroup::Liquidity,
		MetricGroup::Returns,
		MetricGroup::Investment,
		MetricGroup::Shareholders,
		MetricGroup::Valuation,
		MetricGroup::Quality,
		MetricGroup::Market};
}

std::string MetricReport::DescribeGroup(const MetricGroup group)
{
	switch (group)
	{
	case MetricGroup::Revenue:
		return "Выручка и рост";
	case MetricGroup::Profit:
		return "Прибыль и расходы";
	case MetricGroup::Margins:
		return "Маржинальность";
	case MetricGroup::CashFlow:
		return "Денежные потоки";
	case MetricGroup::WorkingCapital:
		return "Оборотный капитал";
	case MetricGroup::Debt:
		return "Долговая нагрузка";
	case MetricGroup::Balance:
		return "Баланс";
	case MetricGroup::Liquidity:
		return "Ликвидность и платежеспособность";
	case MetricGroup::Returns:
		return "Эффективность капитала";
	case MetricGroup::Investment:
		return "Инвестиции и разработки";
	case MetricGroup::Shareholders:
		return "Дивиденды и акционерная отдача";
	case MetricGroup::Valuation:
		return "Оценка стоимости";
	case MetricGroup::Quality:
		return "Качество прибыли";
	case MetricGroup::Market:
		return "Клиенты и рынок";
	}

	return "Прочее";
}
