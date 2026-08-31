#pragma once

#include "finance/MetricFormatter.hpp"
#include "finance/MetricReport.hpp"
#include <string>
#include <vector>

class MarkdownBuilder;

class MarkdownCompanyAnalyticsView
{
public:
	explicit MarkdownCompanyAnalyticsView(MetricFormatOptions options);

	std::string Render(const CompanyAnalytics& analytics) const;

private:
	void RenderTitle(MarkdownBuilder& builder, const CompanyAnalytics& analytics) const;
	void RenderSummary(MarkdownBuilder& builder, const CompanyAnalytics& analytics) const;
	void RenderMetricGroups(MarkdownBuilder& builder, const MetricReport& report) const;
	void RenderSeries(MarkdownBuilder& builder, const CompanyAnalytics& analytics) const;
	void RenderSegments(MarkdownBuilder& builder, const CompanyAnalytics& analytics) const;
	void RenderCompetitors(MarkdownBuilder& builder, const CompanyAnalytics& analytics) const;
	void RenderRisks(MarkdownBuilder& builder, const CompanyAnalytics& analytics) const;
	void RenderRelatedParties(MarkdownBuilder& builder, const CompanyAnalytics& analytics) const;
	void RenderOperations(MarkdownBuilder& builder, const CompanyAnalytics& analytics) const;
	void RenderAssumptions(MarkdownBuilder& builder, const CompanyAnalytics& analytics) const;

	std::vector<std::string> BuildMetricHeaders() const;
	std::vector<std::string> BuildMetricCells(const MetricRow& row) const;

	MetricFormatter m_formatter;
};
