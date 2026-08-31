#pragma once

#include "AnalyticsCommandHandler.hpp"
#include "finance/MetricFormatter.hpp"
#include "view/MarkdownCompanyAnalyticsView.hpp"

class MarkdownReportCommandHandler : public AnalyticsCommandHandler
{
public:
	MarkdownReportCommandHandler(
		std::shared_ptr<CompanyAnalyticsViewModel> viewModel,
		std::filesystem::path outputDirectory,
		MetricFormatOptions formatOptions);

	std::string GetName() const override;
	std::string GetDescription() const override;

protected:
	std::filesystem::path BuildDocument(const CompanyAnalytics& analytics) const override;
	std::string BuildCaption(const CompanyAnalytics& analytics) const override;

private:
	MarkdownCompanyAnalyticsView m_view;
	MetricFormatter m_formatter;
};