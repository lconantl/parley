#include "ConsoleCompanyAnalyticsView.hpp"

#include "finance/Metric.hpp"

#include <iostream>
#include <string>
#include <utility>

namespace
{
constexpr std::size_t TitleWidth = 44;

std::string PadRight(const std::string& text, const std::size_t width)
{
	std::size_t length = 0;

	for (const unsigned char symbol : text)
	{
		if ((symbol & 0xC0) != 0x80)
		{
			++length;
		}
	}

	if (length >= width)
	{
		return text;
	}

	return text + std::string(width - length, ' ');
}

std::string DescribeStatus(const AnalyticsStatus status)
{
	switch (status)
	{
	case AnalyticsStatus::NoData:
		return "данных о компании нет";
	case AnalyticsStatus::EstimatedOnly:
		return "только оценки";
	case AnalyticsStatus::Partial:
		return "частичные данные";
	case AnalyticsStatus::Complete:
		return "полные данные";
	}

	return "статус неизвестен";
}
} // namespace

ConsoleCompanyAnalyticsView::ConsoleCompanyAnalyticsView(MetricFormatOptions options)
	: m_formatter(std::move(options))
{
}

void ConsoleCompanyAnalyticsView::Show(const CompanyAnalytics& analytics) const
{
	Show(analytics, std::cout);
}

void ConsoleCompanyAnalyticsView::Show(
	const CompanyAnalytics& analytics,
	std::ostream& output) const
{
	const MetricReport report(analytics);

	ShowHeader(output, analytics);
	ShowMetrics(output, report);
	ShowSeries(output, analytics);
	ShowAssumptions(output, analytics);
}

void ConsoleCompanyAnalyticsView::ShowHeader(
	std::ostream& output,
	const CompanyAnalytics& analytics)
{
	output << "========================================" << std::endl;
	output << (analytics.name.empty() ? analytics.identifier : analytics.name) << std::endl;

	if (!analytics.activity.empty())
	{
		output << analytics.activity << std::endl;
	}

	output << analytics.year << ", " << DescribeStatus(analytics.status) << std::endl;
	output << "========================================" << std::endl;
}

void ConsoleCompanyAnalyticsView::ShowMetrics(
	std::ostream& output,
	const MetricReport& report) const
{
	for (const MetricGroup group : MetricReport::ListGroups())
	{
		bool printedTitle = false;

		for (const auto& row : report.GetGroup(group))
		{
			if (!m_formatter.ShouldShow(row.value))
			{
				continue;
			}

			if (!printedTitle)
			{
				output << std::endl << "-- " << MetricReport::DescribeGroup(group) << std::endl;
				printedTitle = true;
			}

			output << PadRight(row.title, TitleWidth)
				   << m_formatter.FormatFull(row.value, row.unit)
				   << std::endl;
		}
	}
}

void ConsoleCompanyAnalyticsView::ShowSeries(
	std::ostream& output,
	const CompanyAnalytics& analytics) const
{
	if (analytics.series.empty())
	{
		return;
	}

	output << std::endl << "-- Динамика по годам" << std::endl;

	for (const auto& snapshot : analytics.series)
	{
		output << snapshot.year
			   << "  выручка " << m_formatter.FormatValue(snapshot.revenue, MetricUnit::Money)
			   << ", EBITDA " << m_formatter.FormatValue(snapshot.ebitda, MetricUnit::Money)
			   << ", чистая прибыль "
			   << m_formatter.FormatValue(snapshot.netProfit, MetricUnit::Money)
			   << std::endl;
	}
}

void ConsoleCompanyAnalyticsView::ShowAssumptions(
	std::ostream& output,
	const CompanyAnalytics& analytics)
{
	if (analytics.assumptions.empty())
	{
		return;
	}

	output << std::endl << "-- Допущения" << std::endl;

	std::size_t index = 1;
	for (const auto& assumption : analytics.assumptions)
	{
		output << index++ << ". " << assumption << std::endl;
	}
}
