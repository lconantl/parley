#pragma once

#include "IDocumentBuilder.hpp"
#include "ai/OnePagerNarrator.hpp"
#include "common/output/pdf/model/Slide.hpp"
#include "common/output/pdf/theme/Theme.hpp"
#include "finance/MetricFormatter.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

class OnePagerDocumentBuilder : public IDocumentBuilder
{
public:
	OnePagerDocumentBuilder(
		std::shared_ptr<OnePagerNarrator> narrator,
		std::filesystem::path outputDirectory,
		Theme theme,
		MetricFormatOptions formatOptions);

	std::string GetLabel() const override;
	DocumentBuildResult Build(
		const CompanyAnalytics& analytics,
		const DocumentBuildOptions& options) const override;

private:
	OnePagerNarrative ComposeNarrative(
		const CompanyAnalytics& analytics,
		const AnonymousIdentity& identity) const;

	OnePagerSlideContent BuildContent(
		const CompanyAnalytics& analytics,
		const OnePagerNarrative& narrative,
		const AnonymousIdentity& identity,
		const DocumentBuildOptions& options) const;

	std::string Money(const MetricValue& metric) const;
	std::string Percent(const MetricValue& metric) const;
	std::string Ratio(const MetricValue& metric) const;

	std::vector<KpiEntry> BuildKpis(const CompanyAnalytics& analytics) const;
	std::vector<ValuationBridgeStep> BuildValuationBridge(const CompanyAnalytics& analytics) const;
	std::vector<RevenueMixSegment> BuildRevenueMix(const CompanyAnalytics& analytics) const;
	std::vector<NumberedEntry> BuildBusinessFacts(const CompanyAnalytics& analytics) const;
	std::optional<ChartSlideContent> BuildTrajectoryChart(const CompanyAnalytics& analytics) const;

	std::shared_ptr<OnePagerNarrator> m_narrator;
	std::filesystem::path m_outputDirectory;
	Theme m_theme;
	MetricFormatter m_formatter;
};
