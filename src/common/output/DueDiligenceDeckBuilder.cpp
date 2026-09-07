#include "DueDiligenceDeckBuilder.hpp"
#include "finance/Metric.hpp"
#include <algorithm>
#include <utility>

namespace
{
constexpr auto StatementSources = "бухгалтерская отчетность, ЕГРЮЛ";
constexpr auto MarketSources = "отраслевые обзоры, отчетность компаний отрасли";
constexpr auto MixedSources = "бухгалтерская отчетность, отраслевые обзоры";
constexpr std::size_t MaxCompetitorRows = 5;
constexpr std::size_t MaxSeriesYears = 8;

std::string Dash()
{
	return "—";
}

void AddFigure(
	std::vector<KpiEntry>& figures,
	const std::string& label,
	const std::string& value,
	const std::string& note)
{
	if (value == Dash())
	{
		return;
	}

	figures.push_back({label, value, note});
}

std::vector<NumberedEntry> ToEntries(const std::vector<NarrativePoint>& points)
{
	std::vector<NumberedEntry> entries;
	entries.reserve(points.size());

	for (const auto& point : points)
	{
		entries.push_back({point.title, point.body});
	}

	return entries;
}

std::vector<AnnualSnapshot> TakeRecentYears(const std::vector<AnnualSnapshot>& series)
{
	std::vector<AnnualSnapshot> recent;

	for (const auto& snapshot : series)
	{
		if (recent.size() >= MaxSeriesYears)
		{
			break;
		}

		recent.push_back(snapshot);
	}

	std::reverse(recent.begin(), recent.end());

	return recent;
}

bool HasSeriesValues(const std::vector<double>& values)
{
	for (const double value : values)
	{
		if (std::abs(value) > 0.0)
		{
			return true;
		}
	}

	return false;
}
} // namespace

DueDiligenceDeckBuilder::DueDiligenceDeckBuilder(
	DueDiligenceOptions options,
	MetricFormatOptions formatOptions)
	: m_options(std::move(options))
	, m_formatter(std::move(formatOptions))
{
}

std::string DueDiligenceDeckBuilder::Money(const MetricValue& metric) const
{
	return Metric::IsKnown(metric) ? m_formatter.FormatValue(metric, MetricUnit::Money) : Dash();
}

std::string DueDiligenceDeckBuilder::Percent(const MetricValue& metric) const
{
	return Metric::IsKnown(metric) ? m_formatter.FormatValue(metric, MetricUnit::Percent) : Dash();
}

std::string DueDiligenceDeckBuilder::Ratio(const MetricValue& metric) const
{
	return Metric::IsKnown(metric) ? m_formatter.FormatValue(metric, MetricUnit::Ratio) : Dash();
}

std::string DueDiligenceDeckBuilder::SourceNote(const std::string& sources) const
{
	if (!m_options.showSourceNotes)
	{
		return {};
	}

	return "Источник: " + sources + ", анализ команды";
}

Slide DueDiligenceDeckBuilder::BuildTitle(const AnonymousIdentity& identity) const
{
	TitleSlideContent content;
	content.title = "Инвестиционный анализ";
	content.caption = identity.industry + "  ·  " + identity.region + "  ·  "
		+ identity.scale + "  ·  Период анализа: " + identity.period;

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildExecutiveSummary(
	const DueDiligenceNarrative& narrative) const
{
	BulletSlideContent content;
	content.title = "Ключевые выводы";
	content.bullets = narrative.executiveSummary;

	if (!narrative.investmentVerdict.empty())
	{
		content.bullets.push_back(narrative.investmentVerdict);
	}

	if (content.bullets.empty())
	{
		content.bullets.push_back("Данных недостаточно для формирования выводов.");
	}

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildProfile(
	const CompanyAnalytics& analytics,
	const AnonymousIdentity& identity,
	const DueDiligenceNarrative& narrative) const
{
	ColumnStripSlideContent content;
	content.title = "Профиль компании";
	content.columns = {
		{"Бизнес", narrative.businessProfile.empty() ? identity.industry : narrative.businessProfile},
		{"Масштаб",
			"Выручка за отчетный год " + Money(analytics.revenue.revenue)
				+ ", активы " + Money(analytics.balance.totalAssets)
				+ ", собственный капитал " + Money(analytics.balance.equity) + "."},
		{"География и позиционирование",
			identity.region + ". " + narrative.positioning}};

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildFinancialResults(
	const CompanyAnalytics& analytics,
	const DueDiligenceNarrative& narrative) const
{
	KpiSlideContent content;
	content.title = "Финансовые результаты";

	AddFigure(content.figures, "Выручка", Money(analytics.revenue.revenue), "рост " + Percent(analytics.revenue.revenueGrowth) + " год к году");
	AddFigure(content.figures, "EBITDA", Money(analytics.profit.ebitda), "маржа " + Percent(analytics.margins.ebitdaMargin));
	AddFigure(content.figures, "Чистая прибыль", Money(analytics.profit.netProfit), "маржа " + Percent(analytics.margins.netMargin));
	AddFigure(content.figures, "Свободный поток", Money(analytics.cashFlow.freeCashFlow), "маржа " + Percent(analytics.margins.freeCashFlowMargin));

	content.commentary = narrative.revenueTakeaway;
	content.sourceNote = SourceNote(StatementSources);

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildRevenueChart(
	const CompanyAnalytics& analytics,
	const DueDiligenceNarrative& narrative) const
{
	ChartSlideContent content;
	content.title = "Динамика выручки и прибыли";
	content.kind = ChartKind::GroupedBars;

	ChartSeries revenue{"Выручка", {}};
	ChartSeries profit{"Чистая прибыль", {}};

	for (const auto& snapshot : TakeRecentYears(analytics.series))
	{
		if (!Metric::IsKnown(snapshot.revenue))
		{
			continue;
		}

		content.categories.push_back(std::to_string(snapshot.year));
		revenue.values.push_back(snapshot.revenue.value);
		profit.values.push_back(
			Metric::IsKnown(snapshot.netProfit) ? snapshot.netProfit.value : 0.0);
	}

	content.series.push_back(revenue);

	if (HasSeriesValues(profit.values))
	{
		content.series.push_back(profit);
	}

	content.takeaway = narrative.revenueTakeaway;
	content.sourceNote = SourceNote(StatementSources);

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildMargins(
	const CompanyAnalytics& analytics,
	const DueDiligenceNarrative& narrative) const
{
	TableSlideContent content;
	content.title = "Прибыльность";
	content.headers = {"Показатель", "Значение", "База"};
	content.rows = {
		{"Валовая прибыль", Money(analytics.profit.grossProfit), Percent(analytics.margins.grossMargin)},
		{"Операционная прибыль", Money(analytics.profit.ebit), Percent(analytics.margins.operatingMargin)},
		{"EBITDA", Money(analytics.profit.ebitda), Percent(analytics.margins.ebitdaMargin)},
		{"Чистая прибыль", Money(analytics.profit.netProfit), Percent(analytics.margins.netMargin)},
		{"Операционные расходы", Money(analytics.profit.operatingExpenses), Dash()}};

	content.takeaway = narrative.marginTakeaway;
	content.sourceNote = SourceNote(StatementSources);

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildCashFlow(
	const CompanyAnalytics& analytics,
	const DueDiligenceNarrative& narrative) const
{
	ChartSlideContent content;
	content.title = "Денежный поток";
	content.kind = ChartKind::Line;

	ChartSeries flow{"Свободный денежный поток", {}};

	for (const auto& snapshot : TakeRecentYears(analytics.series))
	{
		if (!Metric::IsKnown(snapshot.freeCashFlow))
		{
			continue;
		}

		content.categories.push_back(std::to_string(snapshot.year));
		flow.values.push_back(snapshot.freeCashFlow.value);
	}

	if (content.categories.size() < 2)
	{
		return BuildQuality(analytics, narrative);
	}

	content.series.push_back(flow);
	content.notes = {
		"Операционный поток: " + Money(analytics.cashFlow.operatingCashFlow),
		"Капитальные затраты: " + Money(analytics.cashFlow.capitalExpenditure),
		"Конверсия прибыли: " + Ratio(analytics.cashFlow.cashConversion)};

	content.takeaway = narrative.cashFlowTakeaway;
	content.sourceNote = SourceNote(StatementSources);

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildBalance(
	const CompanyAnalytics& analytics,
	const DueDiligenceNarrative& narrative) const
{
	TableSlideContent content;
	content.title = "Баланс и долговая нагрузка";
	content.headers = {"Показатель", "Значение", "Норма"};
	content.rows = {
		{"Денежные средства", Money(analytics.cashFlow.cash), Dash()},
		{"Совокупный долг", Money(analytics.debt.totalDebt), Dash()},
		{"Чистый долг", Money(analytics.debt.netDebt), Dash()},
		{"Чистый долг / EBITDA", Ratio(analytics.debt.netDebtToEbitda), "до 3,0x"},
		{"Оборотный капитал", Money(analytics.workingCapital.workingCapital), Dash()},
		{"Текущая ликвидность", Ratio(analytics.liquidity.currentRatio), "1,5–2,5x"},
		{"Коэффициент автономии", Percent(analytics.liquidity.equityRatio), "от 50%"}};

	content.takeaway = narrative.balanceTakeaway;
	content.sourceNote = SourceNote(StatementSources);

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildQuality(
	const CompanyAnalytics& analytics,
	const DueDiligenceNarrative& narrative) const
{
	KpiSlideContent content;
	content.title = "Качество прибыли";

	AddFigure(content.figures, "Разовые доходы", Money(analytics.quality.oneOffIncome), "");
	AddFigure(content.figures, "Разовые расходы", Money(analytics.quality.oneOffExpense), "");
	AddFigure(content.figures, "Нормализованная прибыль", Money(analytics.quality.normalizedNetProfit), "");
	AddFigure(content.figures, "Конверсия в поток", Ratio(analytics.cashFlow.cashConversion), "операционный поток к прибыли");

	content.commentary = narrative.qualityTakeaway;
	content.sourceNote = SourceNote(StatementSources);

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildReturns(
	const CompanyAnalytics& analytics,
	const DueDiligenceNarrative& narrative) const
{
	TableSlideContent content;
	content.title = "Инвестиции и эффективность капитала";
	content.headers = {"Показатель", "Значение", "Комментарий"};
	content.rows = {
		{"Капитальные затраты", Money(analytics.cashFlow.capitalExpenditure), Percent(analytics.investment.capexToRevenue) + " от выручки"},
		{"Разработки", Money(analytics.investment.researchExpense), Percent(analytics.investment.researchToRevenue) + " от выручки"},
		{"ROE", Percent(analytics.returns.returnOnEquity), Dash()},
		{"ROA", Percent(analytics.returns.returnOnAssets), Dash()},
		{"ROIC", Percent(analytics.returns.returnOnInvestedCapital), Dash()},
		{"Оборачиваемость активов", Ratio(analytics.returns.assetTurnover), Dash()}};

	content.takeaway = narrative.returnsTakeaway;
	content.sourceNote = SourceNote(MixedSources);

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildMarket(
	const CompanyAnalytics& analytics,
	const DueDiligenceNarrative& narrative) const
{
	KpiSlideContent content;
	content.title = "Рынок";

	AddFigure(content.figures, "Размер рынка", Money(analytics.market.size), analytics.market.definition);
	AddFigure(content.figures, "Рост рынка", Percent(analytics.market.growth), "в год");
	AddFigure(content.figures, "Доля компании", Percent(analytics.market.share), "");
	AddFigure(content.figures, "Рост компании", Percent(analytics.revenue.revenueGrowth), "год к году");

	content.commentary = narrative.marketCommentary;
	content.sourceNote = SourceNote(MarketSources);

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildCompetitors(
	const CompanyAnalytics& analytics,
	const DueDiligenceNarrative& narrative) const
{
	TableSlideContent content;
	content.title = "Конкурентное окружение";
	content.headers = {"Компания", "Выручка", "Рост", "Маржа", "Оценка"};

	content.rows.push_back({CompanyAnonymizer::SubjectLabel(),
		Money(analytics.revenue.revenue),
		Percent(analytics.revenue.revenueGrowth),
		Percent(analytics.margins.netMargin),
		Money(analytics.valuation.marketCapitalization)});

	const std::vector<Competitor>& competitors = analytics.competitors;

	for (const auto& competitor : competitors)
	{
		if (content.rows.size() > MaxCompetitorRows)
		{
			break;
		}

		content.rows.push_back({competitor.name,
			Money(competitor.revenue),
			Percent(competitor.revenueGrowth),
			Percent(competitor.margin),
			Money(competitor.valuation)});
	}

	content.takeaway = narrative.competitionTakeaway;
	content.sourceNote = SourceNote(MarketSources);

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildRisks(const DueDiligenceNarrative& narrative) const
{
	CardGridSlideContent content;
	content.title = "Ключевые риски";
	content.columnCount = 3;
	content.cards = ToEntries(narrative.risks);

	if (content.cards.empty())
	{
		content.cards.push_back({"Риски не формализованы",
			"Для оценки рисков не хватает данных. Список вопросов приведен далее."});
	}

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildOpportunities(
	const DueDiligenceNarrative& narrative) const
{
	CardGridSlideContent content;
	content.title = "Точки роста";
	content.columnCount = 3;
	content.cards = ToEntries(narrative.opportunities);

	if (content.cards.empty())
	{
		content.cards.push_back({"Потенциал не оценен",
			"Для оценки потенциала не хватает данных о рынке и клиентской базе."});
	}

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildValuation(
	const CompanyAnalytics& analytics,
	const DueDiligenceNarrative& narrative) const
{
	TableSlideContent content;
	content.title = "Оценка стоимости";
	content.headers = {"Показатель", "Значение"};
	content.rows = {
		{"Оценка стоимости бизнеса", Money(analytics.valuation.marketCapitalization)},
		{"Стоимость предприятия", Money(analytics.valuation.enterpriseValue)},
		{"EV / EBITDA", Ratio(analytics.valuation.enterpriseToEbitda)},
		{"EV / Выручка", Ratio(analytics.valuation.enterpriseToRevenue)},
		{"P / E", Ratio(analytics.valuation.priceToEarnings)},
		{"P / FCF", Ratio(analytics.valuation.priceToFreeCashFlow)}};

	content.takeaway = narrative.valuationCommentary;
	content.sourceNote = SourceNote(MarketSources);

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildOpenQuestions(
	const CompanyAnalytics& analytics,
	const DueDiligenceNarrative& narrative) const
{
	BulletSlideContent content;
	content.title = "Что необходимо проверить дополнительно";
	content.bullets = narrative.openQuestions;

	for (const auto& assumption : analytics.assumptions)
	{
		if (content.bullets.size() >= 10)
		{
			break;
		}

		content.bullets.push_back(assumption);
	}

	if (content.bullets.empty())
	{
		content.bullets.push_back("Дополнительных вопросов не выявлено.");
	}

	return Slide{content};
}

Slide DueDiligenceDeckBuilder::BuildConclusion(const DueDiligenceNarrative& narrative) const
{
	ClosingSlideContent content;
	content.title = "Инвестиционный тезис";

	if (!narrative.conclusion.empty())
	{
		content.lines.push_back(narrative.conclusion);
	}

	for (const auto& step : narrative.nextSteps)
	{
		content.lines.push_back(step);
	}

	if (content.lines.empty())
	{
		content.lines.push_back("Решение требует дополнительной проверки данных.");
	}

	return Slide{content};
}

Deck DueDiligenceDeckBuilder::Build(
	const CompanyAnalytics& analytics,
	const DueDiligenceNarrative& narrative) const
{
	const AnonymousIdentity identity = CompanyAnonymizer::Describe(analytics);

	Deck deck;
	deck.title = "Инвестиционный анализ: " + identity.industry;
	deck.author = m_options.author;

	deck.slides.push_back(BuildTitle(identity));
	deck.slides.push_back(BuildExecutiveSummary(narrative));
	deck.slides.push_back(BuildProfile(analytics, identity, narrative));
	deck.slides.push_back(BuildFinancialResults(analytics, narrative));
	deck.slides.push_back(BuildRevenueChart(analytics, narrative));
	deck.slides.push_back(BuildMargins(analytics, narrative));
	deck.slides.push_back(BuildCashFlow(analytics, narrative));
	deck.slides.push_back(BuildBalance(analytics, narrative));
	// deck.slides.push_back(BuildQuality(analytics, narrative));
	deck.slides.push_back(BuildReturns(analytics, narrative));
	deck.slides.push_back(BuildMarket(analytics, narrative));
	deck.slides.push_back(BuildCompetitors(analytics, narrative));
	deck.slides.push_back(BuildRisks(narrative));
	deck.slides.push_back(BuildOpportunities(narrative));
	deck.slides.push_back(BuildValuation(analytics, narrative));
	deck.slides.push_back(BuildOpenQuestions(analytics, narrative));
	// deck.slides.push_back(BuildConclusion(narrative));

	return deck;
}