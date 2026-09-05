#include "OnePagerDocumentBuilder.hpp"

#include "common/output/CompanyAnonymizer.hpp"
#include "common/output/pdf/layout/HaruTextMeasurer.hpp"
#include "common/output/pdf/layout/OnePagerLayout.hpp"
#include "common/output/pdf/render/PdfDocument.hpp"
#include "common/output/pdf/render/PdfGenerator.hpp"
#include "finance/Metric.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
constexpr auto Label = "Одностраничник";
constexpr auto FileExtension = ".pdf";
constexpr auto MimeType = "application/pdf";
constexpr std::size_t MaxKpis = 5;
constexpr std::size_t MaxBusinessFacts = 4;
constexpr std::size_t MaxRisks = 3;
constexpr std::size_t MinSegmentsForMix = 2;
constexpr std::size_t MaxTrajectoryYears = 6;
constexpr std::size_t MinTrajectoryYears = 2;
constexpr double MinPageHeight = 540.0;

double MeasureRequiredPageHeight(const Theme& theme, const OnePagerSlideContent& content)
{
	PdfDocument scratchDocument;
	std::map<FontRole, HPDF_Font> fonts;
	for (const auto& [role, path] : theme.fonts)
	{
		fonts[role] = scratchDocument.LoadFont(path);
	}

	const HaruTextMeasurer measurer(fonts);
	const OnePagerLayout layout(theme, measurer);

	return std::max(MinPageHeight, layout.MeasureRequiredHeight(content));
}

Theme AdjustPageHeight(Theme theme, const double height)
{
	theme.metrics.page.height = height;

	return theme;
}

void PrepareDirectory(const std::filesystem::path& directory)
{
	if (directory.empty() || std::filesystem::exists(directory))
	{
		return;
	}

	std::filesystem::create_directories(directory);
}

void AssertIsNarratorValid(const std::shared_ptr<OnePagerNarrator>& narrator)
{
	if (narrator == nullptr)
	{
		throw std::invalid_argument("Составитель текстовой части не может быть пустым");
	}
}

std::string BuildFileName(const CompanyAnalytics& analytics, const bool anonymize)
{
	const std::string identifier = (!anonymize && !analytics.identifier.empty())
		? analytics.identifier
		: "target";

	return "op-" + identifier + "-" + std::to_string(analytics.year) + FileExtension;
}

MetricFormatOptions MakeCompactOptions(MetricFormatOptions options)
{
	options.compactMoney = true;

	return options;
}

std::string MaskIfNeeded(const std::string& text, const CompanyAnalytics& analytics, const bool anonymize)
{
	return anonymize ? CompanyAnonymizer::MaskText(text, analytics) : text;
}
} // namespace

OnePagerDocumentBuilder::OnePagerDocumentBuilder(
	std::shared_ptr<OnePagerNarrator> narrator,
	std::filesystem::path outputDirectory,
	Theme theme,
	MetricFormatOptions formatOptions)
	: m_narrator(std::move(narrator))
	, m_outputDirectory(std::move(outputDirectory))
	, m_theme(std::move(theme))
	, m_formatter(MakeCompactOptions(formatOptions))
{
	AssertIsNarratorValid(m_narrator);
	PrepareDirectory(m_outputDirectory);
}

std::string OnePagerDocumentBuilder::GetLabel() const
{
	return Label;
}

std::string OnePagerDocumentBuilder::Money(const MetricValue& metric) const
{
	return Metric::IsKnown(metric) ? m_formatter.FormatValue(metric, MetricUnit::Money) : std::string();
}

std::string OnePagerDocumentBuilder::Percent(const MetricValue& metric) const
{
	return Metric::IsKnown(metric) ? m_formatter.FormatValue(metric, MetricUnit::Percent) : std::string();
}

std::string OnePagerDocumentBuilder::Ratio(const MetricValue& metric) const
{
	return Metric::IsKnown(metric) ? m_formatter.FormatValue(metric, MetricUnit::Ratio) : std::string();
}

std::vector<KpiEntry> OnePagerDocumentBuilder::BuildKpis(const CompanyAnalytics& analytics) const
{
	std::vector<KpiEntry> kpis;

	if (Metric::IsKnown(analytics.revenue.revenue))
	{
		std::string note;
		if (Metric::IsKnown(analytics.revenue.revenueGrowth))
		{
			note = "рост " + Percent(analytics.revenue.revenueGrowth) + " г/г";
		}
		else if (Metric::IsKnown(analytics.revenue.revenueCagr))
		{
			note = "CAGR " + Percent(analytics.revenue.revenueCagr) + " за 3 года";
		}

		kpis.push_back({"Выручка", Money(analytics.revenue.revenue), note, false});
	}

	if (Metric::IsKnown(analytics.profit.ebitda))
	{
		const std::string note = Metric::IsKnown(analytics.margins.ebitdaMargin)
			? "маржа " + Percent(analytics.margins.ebitdaMargin)
			: std::string();

		kpis.push_back({"EBITDA", Money(analytics.profit.ebitda), note, false});
	}

	if (Metric::IsKnown(analytics.margins.ebitdaMargin))
	{
		kpis.push_back({"Маржа EBITDA", Percent(analytics.margins.ebitdaMargin), "", false});
	}

	if (Metric::IsKnown(analytics.cashFlow.freeCashFlow))
	{
		const std::string note = Metric::IsKnown(analytics.cashFlow.capitalExpenditure)
			? "CAPEX " + Money(analytics.cashFlow.capitalExpenditure)
			: std::string();

		kpis.push_back({
			"Свободный денежный поток",
			Money(analytics.cashFlow.freeCashFlow),
			note,
			analytics.cashFlow.freeCashFlow.value < 0.0});
	}

	if (Metric::IsKnown(analytics.debt.netDebtToEbitda))
	{
		kpis.push_back({
			"Чистый долг / EBITDA",
			Ratio(analytics.debt.netDebtToEbitda),
			"фактор создания стоимости",
			false});
	}

	if (kpis.size() > MaxKpis)
	{
		kpis.resize(MaxKpis);
	}

	return kpis;
}

std::vector<ValuationBridgeStep> OnePagerDocumentBuilder::BuildValuationBridge(
	const CompanyAnalytics& analytics) const
{
	if (!Metric::IsKnown(analytics.valuation.enterpriseValue)
		|| !Metric::IsKnown(analytics.debt.netDebt)
		|| !Metric::IsKnown(analytics.valuation.marketCapitalization))
	{
		return {};
	}

	const std::string multiple = Metric::IsKnown(analytics.valuation.enterpriseToEbitda)
		? Ratio(analytics.valuation.enterpriseToEbitda) + " EBITDA"
		: std::string();

	std::vector<ValuationBridgeStep> steps;
	steps.push_back({"Enterprise Value", Money(analytics.valuation.enterpriseValue), multiple, false});
	steps.push_back({"- Чистый долг", Money(analytics.debt.netDebt), "", false});
	steps.push_back({"= Equity Value (100%)", Money(analytics.valuation.marketCapitalization), "", true});

	return steps;
}

std::vector<RevenueMixSegment> OnePagerDocumentBuilder::BuildRevenueMix(
	const CompanyAnalytics& analytics) const
{
	std::vector<RevenueMixSegment> usable;

	for (const auto& segment : analytics.segments)
	{
		if (!Metric::IsKnown(segment.revenue))
		{
			continue;
		}

		double fraction = 0.0;
		if (Metric::IsKnown(segment.revenueShare))
		{
			fraction = segment.revenueShare.value / 100.0;
		}
		else if (Metric::IsKnown(analytics.revenue.revenue) && analytics.revenue.revenue.value > 0.0)
		{
			fraction = segment.revenue.value / analytics.revenue.revenue.value;
		}

		if (fraction <= 0.0)
		{
			continue;
		}

		const std::string share = Metric::IsKnown(segment.revenueShare)
			? Percent(segment.revenueShare)
			: MetricFormatter::FormatNumber(fraction * 100.0, 0) + " %";

		usable.push_back({segment.name, share, Money(segment.revenue), fraction});
	}

	if (usable.size() < MinSegmentsForMix)
	{
		return {};
	}

	return usable;
}

std::vector<NumberedEntry> OnePagerDocumentBuilder::BuildBusinessFacts(
	const CompanyAnalytics& analytics) const
{
	std::vector<NumberedEntry> facts;

	if (Metric::IsKnown(analytics.customers.count))
	{
		facts.push_back({"Клиентская база", MetricFormatter::FormatNumber(analytics.customers.count.value, 0)});
	}

	if (Metric::IsKnown(analytics.market.size))
	{
		std::string body = Money(analytics.market.size);
		if (Metric::IsKnown(analytics.market.growth))
		{
			body += ", рост " + Percent(analytics.market.growth) + " в год";
		}

		facts.push_back({"Рынок", body});
	}

	if (Metric::IsKnown(analytics.market.share))
	{
		facts.push_back({"Доля рынка", Percent(analytics.market.share)});
	}

	for (const auto& operation : analytics.operations)
	{
		if (facts.size() >= MaxBusinessFacts)
		{
			break;
		}

		if (!Metric::IsKnown(operation.value))
		{
			continue;
		}

		std::string body = MetricFormatter::FormatNumber(operation.value.value, 1);
		if (!operation.unit.empty())
		{
			body += " " + operation.unit;
		}

		facts.push_back({operation.name, body});
	}

	if (facts.size() > MaxBusinessFacts)
	{
		facts.resize(MaxBusinessFacts);
	}

	return facts;
}

std::optional<ChartSlideContent> OnePagerDocumentBuilder::BuildTrajectoryChart(
	const CompanyAnalytics& analytics) const
{
	std::vector<AnnualSnapshot> recent;

	for (const auto& snapshot : analytics.series)
	{
		if (recent.size() >= MaxTrajectoryYears)
		{
			break;
		}

		if (!Metric::IsKnown(snapshot.revenue))
		{
			continue;
		}

		recent.push_back(snapshot);
	}

	std::reverse(recent.begin(), recent.end());

	if (recent.size() < MinTrajectoryYears)
	{
		return std::nullopt;
	}

	ChartSlideContent chart;
	chart.title = "Финансовая динамика";
	chart.kind = ChartKind::GroupedBars;

	ChartSeries revenue{"Выручка", {}};
	ChartSeries ebitda{"EBITDA", {}};
	std::vector<double> marginValues;
	bool hasMargin = true;

	for (const auto& snapshot : recent)
	{
		chart.categories.push_back(std::to_string(snapshot.year));
		revenue.values.push_back(snapshot.revenue.value);

		const double ebitdaValue = Metric::IsKnown(snapshot.ebitda) ? snapshot.ebitda.value : 0.0;
		ebitda.values.push_back(ebitdaValue);

		if (Metric::IsKnown(snapshot.ebitda) && snapshot.revenue.value > 0.0)
		{
			marginValues.push_back(ebitdaValue / snapshot.revenue.value * 100.0);
		}
		else
		{
			hasMargin = false;
		}
	}

	chart.series.push_back(revenue);
	chart.series.push_back(ebitda);

	if (hasMargin)
	{
		chart.secondarySeries = SecondaryAxisSeries{"Маржа EBITDA", marginValues, "%"};
	}

	return chart;
}

OnePagerNarrative OnePagerDocumentBuilder::ComposeNarrative(
	const CompanyAnalytics& analytics,
	const AnonymousIdentity& identity) const
{
	try
	{
		return m_narrator->Compose(analytics, identity);
	}
	catch (const std::exception& error)
	{
		std::cout << "one-pager narrative: не удалось подготовить текстовую часть: "
				  << error.what() << std::endl;
	}

	return OnePagerNarrator::BuildFallback(analytics);
}

OnePagerSlideContent OnePagerDocumentBuilder::BuildContent(
	const CompanyAnalytics& analytics,
	const OnePagerNarrative& narrative,
	const AnonymousIdentity& identity,
	const DocumentBuildOptions& options) const
{
	OnePagerSlideContent content;

	content.eyebrow = identity.industry + " \xC2\xB7 " + identity.region
		+ (options.anonymize ? " \xC2\xB7 обезличенный актив" : "");
	content.vintageLabel = "ДАННЫЕ " + std::to_string(analytics.year) + " ГОДА";
	content.headline = MaskIfNeeded(narrative.headline, analytics, options.anonymize);

	std::vector<std::string> subtitleParts;
	if (Metric::IsKnown(analytics.revenue.revenue))
	{
		subtitleParts.push_back(Money(analytics.revenue.revenue) + " выручка");
	}
	if (Metric::IsKnown(analytics.margins.ebitdaMargin))
	{
		subtitleParts.push_back(Percent(analytics.margins.ebitdaMargin) + " маржа EBITDA");
	}
	if (Metric::IsKnown(analytics.debt.netDebtToEbitda))
	{
		subtitleParts.push_back(Ratio(analytics.debt.netDebtToEbitda) + " чистый долг/EBITDA");
	}
	if (Metric::IsKnown(analytics.customers.count))
	{
		subtitleParts.push_back(MetricFormatter::FormatNumber(analytics.customers.count.value, 0) + " клиентов");
	}

	for (std::size_t index = 0; index < subtitleParts.size(); ++index)
	{
		if (index > 0)
		{
			content.subtitle += "   |   ";
		}
		content.subtitle += subtitleParts[index];
	}

	content.kpis = BuildKpis(analytics);
	content.valuationBridge = BuildValuationBridge(analytics);

	if (!content.valuationBridge.empty() && !narrative.valuationTakeaway.empty())
	{
		content.valuationMultipleNote = MaskIfNeeded(narrative.valuationTakeaway, analytics, options.anonymize);
	}

	content.revenueMix = BuildRevenueMix(analytics);
	content.businessFacts = BuildBusinessFacts(analytics);
	content.trajectory = BuildTrajectoryChart(analytics);

	for (const auto& thesis : narrative.investmentCase)
	{
		content.investmentCase.push_back({thesis.title, thesis.body});
	}

	if (content.investmentCase.empty())
	{
		content.investmentCase.push_back({
			"Требуется дополнительная проверка",
			"Данных недостаточно для инвестиционного тезиса без риска домысливания."});
	}

	for (auto& entry : content.investmentCase)
	{
		entry.title = MaskIfNeeded(entry.title, analytics, options.anonymize);
		entry.body = MaskIfNeeded(entry.body, analytics, options.anonymize);
	}

	if (Metric::IsKnown(analytics.debt.netDebtToEbitda))
	{
		content.valueCreationSteps.push_back("Чистый долг/EBITDA " + Ratio(analytics.debt.netDebtToEbitda));
	}

	for (const auto& action : narrative.valueCreationActions)
	{
		content.valueCreationSteps.push_back(MaskIfNeeded(action, analytics, options.anonymize));
	}

	for (const auto& risk : analytics.legalRisks)
	{
		if (content.risks.size() >= MaxRisks)
		{
			break;
		}

		content.risks.push_back({risk.title, risk.description});
	}

	for (const auto& risk : narrative.keyRisks)
	{
		if (content.risks.size() >= MaxRisks)
		{
			break;
		}

		content.risks.push_back({
			MaskIfNeeded(risk.title, analytics, options.anonymize),
			MaskIfNeeded(risk.body, analytics, options.anonymize)});
	}

	content.nextStep = narrative.nextStep.empty()
		? "NDA -> доступ к данным -> встреча с менеджментом -> индикативное предложение"
		: MaskIfNeeded(narrative.nextStep, analytics, options.anonymize);

	content.sourceNote = options.showSourceNotes
		? "Источник: бухгалтерская отчетность, ЕГРЮЛ, анализ команды"
		: "";

	return content;
}

DocumentBuildResult OnePagerDocumentBuilder::Build(
	const CompanyAnalytics& analytics,
	const DocumentBuildOptions& options) const
{
	const AnonymousIdentity identity = CompanyAnonymizer::Describe(analytics);
	const OnePagerNarrative narrative = ComposeNarrative(analytics, identity);
	const OnePagerSlideContent content = BuildContent(analytics, narrative, identity, options);

	Deck deck;
	deck.title = "Инвестиционный тизер: " + identity.industry;
	deck.author = options.author;
	deck.slides.push_back(Slide{content});

	const Theme pageTheme = AdjustPageHeight(m_theme, MeasureRequiredPageHeight(m_theme, content));

	const std::filesystem::path path = m_outputDirectory / BuildFileName(analytics, options.anonymize);
	PdfGenerator::Generate(deck, pageTheme, path);

	const std::string caption = "Одностраничник\n" + identity.industry + "\n"
		+ identity.region + ", период " + identity.period;

	return {path, caption, MimeType};
}
