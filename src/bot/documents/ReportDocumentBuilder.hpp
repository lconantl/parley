#pragma once

#include "IDocumentBuilder.hpp"
#include "finance/MetricFormatter.hpp"
#include "view/MarkdownCompanyAnalyticsView.hpp"

#include <filesystem>

class ReportDocumentBuilder : public IDocumentBuilder
{
public:
	ReportDocumentBuilder(std::filesystem::path outputDirectory, MetricFormatOptions formatOptions);

	std::string GetLabel() const override;
	DocumentBuildResult Build(
		const CompanyAnalytics& analytics,
		const DocumentBuildOptions& options) const override;

private:
	std::filesystem::path m_outputDirectory;
	MarkdownCompanyAnalyticsView m_view;
	MetricFormatter m_formatter;
};
