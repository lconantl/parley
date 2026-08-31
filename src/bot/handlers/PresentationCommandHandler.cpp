#include "PresentationCommandHandler.hpp"
#include "common/output/pdf/render/PdfGenerator.hpp"
#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
constexpr auto CommandName = "pres";
constexpr auto CommandDescription = "Инвестиционный анализ в PDF";
constexpr auto FileExtension = ".pdf";
constexpr auto MimeType = "application/pdf";

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

PresentationCommandHandler::PresentationCommandHandler(
	std::shared_ptr<CompanyAnalyticsViewModel> viewModel,
	std::shared_ptr<DueDiligenceNarrator> narrator,
	std::filesystem::path outputDirectory,
	Theme theme,
	DueDiligenceOptions reportOptions,
	MetricFormatOptions formatOptions)
	: AnalyticsCommandHandler(std::move(viewModel), std::move(outputDirectory))
	, m_narrator(std::move(narrator))
	, m_theme(std::move(theme))
	, m_builder(std::move(reportOptions), std::move(formatOptions))
{
	AssertIsNarratorValid(m_narrator);
}

std::string PresentationCommandHandler::GetName() const
{
	return CommandName;
}

std::string PresentationCommandHandler::GetDescription() const
{
	return CommandDescription;
}

std::string PresentationCommandHandler::GetMimeType() const
{
	return MimeType;
}

DueDiligenceNarrative PresentationCommandHandler::ComposeNarrative(
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

std::filesystem::path PresentationCommandHandler::BuildDocument(
	const CompanyAnalytics& analytics) const
{
	const DueDiligenceNarrative narrative = ComposeNarrative(analytics);
	const Deck deck = m_builder.Build(analytics, narrative);
	const std::filesystem::path path = GetOutputDirectory() / BuildFileName(analytics);

	PdfGenerator::Generate(deck, m_theme, path);

	return path;
}

std::string PresentationCommandHandler::BuildCaption(const CompanyAnalytics& analytics) const
{
	const AnonymousIdentity identity = CompanyAnonymizer::Describe(analytics);

	return "Инвестиционный анализ\n"
		+ identity.industry + "\n"
		+ identity.region + ", период " + identity.period;
}