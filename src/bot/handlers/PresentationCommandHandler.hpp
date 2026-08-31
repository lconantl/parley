#pragma once

#include "AnalyticsCommandHandler.hpp"
#include "ai/DueDiligenceNarrator.hpp"
#include "common/output/DueDiligenceDeckBuilder.hpp"
#include "common/output/pdf/theme/Theme.hpp"
#include "finance/MetricFormatter.hpp"
#include <memory>

class PresentationCommandHandler : public AnalyticsCommandHandler
{
public:
	PresentationCommandHandler(
		std::shared_ptr<CompanyAnalyticsViewModel> viewModel,
		std::shared_ptr<DueDiligenceNarrator> narrator,
		std::filesystem::path outputDirectory,
		Theme theme,
		DueDiligenceOptions reportOptions,
		MetricFormatOptions formatOptions);

	std::string GetName() const override;
	std::string GetDescription() const override;

protected:
	std::filesystem::path BuildDocument(const CompanyAnalytics& analytics) const override;
	std::string BuildCaption(const CompanyAnalytics& analytics) const override;
	std::string GetMimeType() const override;

private:
	DueDiligenceNarrative ComposeNarrative(const CompanyAnalytics& analytics) const;

	std::shared_ptr<DueDiligenceNarrator> m_narrator;
	Theme m_theme;
	DueDiligenceDeckBuilder m_builder;
};