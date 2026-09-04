#pragma once

#include "IDocumentBuilder.hpp"
#include "ai/DueDiligenceNarrator.hpp"
#include "common/output/pdf/theme/Theme.hpp"
#include "finance/MetricFormatter.hpp"

#include <filesystem>
#include <memory>

class PresentationDocumentBuilder : public IDocumentBuilder
{
public:
	PresentationDocumentBuilder(
		std::shared_ptr<DueDiligenceNarrator> narrator,
		std::filesystem::path outputDirectory,
		Theme theme,
		MetricFormatOptions formatOptions);

	std::string GetLabel() const override;
	DocumentBuildResult Build(
		const CompanyAnalytics& analytics,
		const DocumentBuildOptions& options) const override;

private:
	DueDiligenceNarrative ComposeNarrative(const CompanyAnalytics& analytics) const;

	std::shared_ptr<DueDiligenceNarrator> m_narrator;
	std::filesystem::path m_outputDirectory;
	Theme m_theme;
	MetricFormatOptions m_formatOptions;
};
