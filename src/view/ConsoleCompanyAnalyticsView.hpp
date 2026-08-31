#pragma once

#include "finance/MetricFormatter.hpp"
#include "finance/MetricReport.hpp"
#include <iosfwd>

class ConsoleCompanyAnalyticsView
{
public:
	explicit ConsoleCompanyAnalyticsView(MetricFormatOptions options);

	void Show(const CompanyAnalytics& analytics) const;
	void Show(const CompanyAnalytics& analytics, std::ostream& output) const;

private:
	static void ShowHeader(std::ostream& output, const CompanyAnalytics& analytics);
	void ShowMetrics(std::ostream& output, const MetricReport& report) const;
	void ShowSeries(std::ostream& output, const CompanyAnalytics& analytics) const;
	static void ShowAssumptions(std::ostream& output, const CompanyAnalytics& analytics);

	MetricFormatter m_formatter;
};
