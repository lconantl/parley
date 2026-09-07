#include "MetricFormatter.hpp"
#include "Metric.hpp"
#include <cmath>
#include <sstream>
#include <utility>

namespace
{
constexpr double Thousand = 1000.0;
constexpr double Million = 1000000.0;
constexpr double Billion = 1000000000.0;
constexpr double PercentScale = 100.0;
constexpr int DefaultDecimals = 2;
constexpr int PercentDecimals = 1;

std::string ReplaceDecimalSeparator(std::string text)
{
	const std::size_t position = text.find('.');
	if (position != std::string::npos)
	{
		text[position] = ',';
	}

	return text;
}

std::string InsertGroupSeparators(const std::string& text)
{
	const std::size_t decimal = text.find(',');
	const std::size_t integerEnd = decimal == std::string::npos ? text.size() : decimal;
	const bool negative = !text.empty() && text.front() == '-';
	const std::size_t start = negative ? 1 : 0;

	std::string digits = text.substr(start, integerEnd - start);
	std::string grouped;

	for (std::size_t index = 0; index < digits.size(); ++index)
	{
		const std::size_t remaining = digits.size() - index;
		if (index > 0 && remaining % 3 == 0)
		{
			grouped += ' ';
		}

		grouped += digits[index];
	}

	return (negative ? "-" : "") + grouped + text.substr(integerEnd);
}

std::string BuildFixed(const double value, const int decimals)
{
	std::ostringstream stream;
	stream.precision(decimals);
	stream << std::fixed << value;

	return ReplaceDecimalSeparator(stream.str());
}

std::string FormatCompactMoney(const double value)
{
	const double magnitude = std::abs(value);

	if (magnitude >= Billion)
	{
		return BuildFixed(value / Billion, DefaultDecimals) + " млрд руб.";
	}

	if (magnitude >= Million)
	{
		return BuildFixed(value / Million, DefaultDecimals) + " млн руб.";
	}

	if (magnitude >= Thousand)
	{
		return BuildFixed(value / Thousand, DefaultDecimals) + " тыс руб.";
	}

	return BuildFixed(value, DefaultDecimals) + " руб.";
}

std::string FormatByUnit(const double value, const MetricUnit unit, const bool compactMoney, const bool moneyInMillions)
{
	switch (unit)
	{
	case MetricUnit::Money:
		return moneyInMillions
			? MetricFormatter::FormatMoneyInMillions(value)
			: MetricFormatter::FormatMoney(value, compactMoney);
	case MetricUnit::Percent:
		return MetricFormatter::FormatNumber(value, PercentDecimals) + " %";
	case MetricUnit::Ratio:
		return MetricFormatter::FormatNumber(value, DefaultDecimals) + "x";
	case MetricUnit::Days:
		return MetricFormatter::FormatNumber(value, PercentDecimals) + " дн.";
	case MetricUnit::Count:
		return MetricFormatter::FormatNumber(value, 0);
	case MetricUnit::Text:
		return MetricFormatter::FormatNumber(value, DefaultDecimals);
	}

	return MetricFormatter::FormatNumber(value, DefaultDecimals);
}
} // namespace

MetricFormatter::MetricFormatter(MetricFormatOptions options)
	: m_options(std::move(options))
{
}

bool MetricFormatter::ShouldShow(const MetricValue& metric) const
{
	if (metric.origin == MetricOrigin::Unavailable)
	{
		return m_options.showMissing;
	}

	if (metric.origin == MetricOrigin::NotApplicable)
	{
		return m_options.showNotApplicable;
	}

	return true;
}

bool MetricFormatter::ShowsOrigin() const noexcept
{
	return m_options.showOrigin;
}

bool MetricFormatter::ShowsConfidence() const noexcept
{
	return m_options.showConfidence;
}

std::string MetricFormatter::FormatMoney(const double value, const bool compact)
{
	if (compact)
	{
		return FormatCompactMoney(value);
	}

	return InsertGroupSeparators(BuildFixed(value, DefaultDecimals)) + " ₽";
}

std::string MetricFormatter::FormatMoneyInMillions(const double value)
{
	return InsertGroupSeparators(BuildFixed(value / Million, DefaultDecimals)) + " млн ₽";
}

std::string MetricFormatter::FormatNumber(const double value, const int decimals)
{
	return InsertGroupSeparators(BuildFixed(value, decimals));
}

std::string MetricFormatter::FormatValue(const MetricValue& metric, const MetricUnit unit) const
{
	if (metric.origin == MetricOrigin::NotApplicable)
	{
		return "—";
	}

	if (!Metric::IsKnown(metric))
	{
		return m_options.missingText;
	}

	return FormatByUnit(metric.value, unit, m_options.compactMoney, m_options.moneyInMillions);
}

std::string MetricFormatter::FormatOrigin(const MetricValue& metric) const
{
	if (!m_options.showOrigin || !Metric::IsKnown(metric))
	{
		return {};
	}

	return Metric::DescribeOrigin(metric.origin);
}

std::string MetricFormatter::FormatConfidence(const MetricValue& metric) const
{
	if (!m_options.showConfidence || !Metric::IsKnown(metric))
	{
		return {};
	}

	return FormatNumber(metric.confidence * PercentScale, 0) + " %";
}

std::string MetricFormatter::FormatFull(const MetricValue& metric, const MetricUnit unit) const
{
	std::string text = FormatValue(metric, unit);

	const std::string origin = FormatOrigin(metric);
	const std::string confidence = FormatConfidence(metric);

	if (origin.empty() && confidence.empty())
	{
		return text;
	}

	text += " (";
	text += origin;

	if (!origin.empty() && !confidence.empty())
	{
		text += ", ";
	}

	text += confidence;
	text += ")";

	if (m_options.showComment && !metric.comment.empty())
	{
		text += " — " + metric.comment;
	}

	return text;
}
