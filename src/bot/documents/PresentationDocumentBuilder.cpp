#include "PresentationDocumentBuilder.hpp"

#include "common/output/CompanyAnonymizer.hpp"
#include "common/output/DueDiligenceDeckBuilder.hpp"
#include "common/output/pdf/render/PdfGenerator.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
constexpr auto Label = "Презентация";
constexpr auto FileExtension = ".pdf";
constexpr auto MimeType = "application/pdf";

void PrepareDirectory(const std::filesystem::path& directory)
{
	if (directory.empty() || std::filesystem::exists(directory))
	{
		return;
	}

	std::filesystem::create_directories(directory);
}

void AssertIsNarratorValid(const std::shared_ptr<DueDiligenceNarrator>& narrator)
{
	if (narrator == nullptr)
	{
		throw std::invalid_argument("Составитель текстовой части не может быть пустым");
	}
}

std::string BuildFileName(const CompanyAnalytics& analytics)
{
	const std::string identifier = analytics.identifier.empty() ? "target" : analytics.identifier;

	return "dd-" + identifier + "-" + std::to_string(analytics.year) + FileExtension;
}
} // namespace

PresentationDocumentBuilder::PresentationDocumentBuilder(
	std::shared_ptr<DueDiligenceNarrator> narrator,
	std::filesystem::path outputDirectory,
	Theme theme,
	MetricFormatOptions formatOptions)
	: m_narrator(std::move(narrator))
	, m_outputDirectory(std::move(outputDirectory))
	, m_theme(std::move(theme))
	, m_formatOptions(formatOptions)
{
	AssertIsNarratorValid(m_narrator);
	PrepareDirectory(m_outputDirectory);
}

std::string PresentationDocumentBuilder::GetLabel() const
{
	return Label;
}

DueDiligenceNarrative PresentationDocumentBuilder::ComposeNarrative(
	const CompanyAnalytics& analytics) const
{
	const AnonymousIdentity identity = CompanyAnonymizer::Describe(analytics);

	try
	{
		return m_narrator->Compose(analytics, identity);
	}
	catch (const std::exception& error)
	{
		std::cout << "narrative: не удалось подготовить текстовую часть: "
				  << error.what() << std::endl;
	}

	return DueDiligenceNarrator::BuildFallback(analytics);
}

DocumentBuildResult PresentationDocumentBuilder::Build(
	const CompanyAnalytics& analytics,
	const DocumentBuildOptions& options) const
{
	const DueDiligenceOptions deckOptions{options.anonymize, options.showSourceNotes, options.author};
	const DueDiligenceDeckBuilder builder(deckOptions, m_formatOptions);

	const DueDiligenceNarrative narrative = ComposeNarrative(analytics);
	const Deck deck = builder.Build(analytics, narrative);

	const std::filesystem::path path = m_outputDirectory / BuildFileName(analytics);
	PdfGenerator::Generate(deck, m_theme, path);

	const AnonymousIdentity identity = CompanyAnonymizer::Describe(analytics);
	const std::string caption = "Инвестиционный анализ\n"
		+ identity.industry + "\n"
		+ identity.region + ", период " + identity.period;

	return {path, caption, MimeType};
}
