#include "ai/OnePagerNarrator.hpp"
#include "ai/PolzaClient.hpp"
#include "bot/documents/OnePagerDocumentBuilder.hpp"
#include "common/output/pdf/theme/StrategyPartnersTheme.hpp"
#include "finance/CompanyAnalytics.hpp"
#include "finance/Metric.hpp"
#include "finance/MetricFormatter.hpp"

#include <filesystem>
#include <iostream>
#include <memory>

namespace
{
CompanyAnalytics BuildFixture()
{
	CompanyAnalytics analytics;
	analytics.identifier = "7712345678";
	analytics.name = "ООО Тестовая Компания";
	analytics.activity = "61.10 Деятельность в области связи на базе проводных технологий";
	analytics.region = "г. Москва, Московская область";
	analytics.legalForm = "ООО";
	analytics.year = 2025;
	analytics.status = AnalyticsStatus::Partial;

	analytics.revenue.revenue = Metric::Reported(61000000000.0);
	analytics.revenue.revenueGrowth = Metric::Computed(8.8, 1.0);
	analytics.revenue.revenueCagr = Metric::Computed(9.9, 1.0);

	analytics.profit.ebitda = Metric::Reported(25300000000.0);
	analytics.margins.ebitdaMargin = Metric::Computed(41.5, 1.0);

	analytics.cashFlow.freeCashFlow = Metric::Reported(-5900000000.0);
	analytics.cashFlow.capitalExpenditure = Metric::Reported(6200000000.0);

	analytics.debt.netDebt = Metric::Computed(143600000000.0, 1.0);
	analytics.debt.netDebtToEbitda = Metric::Computed(5.67, 1.0);

	analytics.valuation.enterpriseValue = Metric::Estimated(161200000000.0, 0.6, "оценка по мультипликаторам");
	analytics.valuation.enterpriseToEbitda = Metric::Estimated(6.4, 0.6, "оценка");
	analytics.valuation.marketCapitalization = Metric::Estimated(17600000000.0, 0.6, "оценка");

	analytics.customers.count = Metric::Reported(5000000.0);

	analytics.market.definition = "Услуги фиксированной связи и ШПД в РФ";
	analytics.market.size = Metric::Estimated(623000000000.0, 0.5, "оценка");
	analytics.market.growth = Metric::Estimated(5.0, 0.5, "оценка");
	analytics.market.share = Metric::Estimated(9.8, 0.5, "оценка");

	BusinessSegment b2b;
	b2b.name = "B2B";
	b2b.revenue = Metric::Reported(28200000000.0);
	b2b.revenueShare = Metric::Computed(46.0, 1.0);

	BusinessSegment b2c;
	b2c.name = "B2C";
	b2c.revenue = Metric::Reported(21900000000.0);
	b2c.revenueShare = Metric::Computed(36.0, 1.0);

	BusinessSegment enterprise;
	enterprise.name = "Enterprise";
	enterprise.revenue = Metric::Reported(11000000000.0);
	enterprise.revenueShare = Metric::Computed(18.0, 1.0);

	analytics.segments = {b2b, b2c, enterprise};

	analytics.legalRisks.push_back(
		{"Высокая долговая нагрузка",
			"Рефинансирование 146,5 млрд руб. в течение двух лет создает существенную зависимость от рыночных условий "
			"привлечения долгового капитала и может ограничить гибкость менеджмента при неблагоприятной конъюнктуре.",
			"high", "ЕГРЮЛ"});
	analytics.legalRisks.push_back(
		{"Концентрация рынка",
			"Топ-5 операторов контролируют около 70% рынка, что создает риск ценовых войн и снижения маржинальности "
			"при обострении конкуренции за крупных корпоративных клиентов.",
			"medium", "отраслевой обзор"});
	analytics.legalRisks.push_back(
		{"Регуляторные изменения",
			"Обсуждаемые поправки в отраслевое регулирование могут повлиять на тарифную политику и потребовать "
			"дополнительных капитальных затрат на соответствие новым требованиям.",
			"medium", "анализ отрасли"});

	const struct
	{
		int year;
		double revenue;
		double ebitda;
	} years[] = {
		{2025, 61000000000.0, 25300000000.0},
		{2024, 57000000000.0, 22500000000.0},
		{2023, 54000000000.0, 20000000000.0},
		{2022, 50000000000.0, 18000000000.0},
		{2021, 45000000000.0, 16000000000.0},
	};

	for (const auto& year : years)
	{
		AnnualSnapshot snapshot;
		snapshot.year = year.year;
		snapshot.revenue = Metric::Reported(year.revenue);
		snapshot.ebitda = Metric::Reported(year.ebitda);
		analytics.series.push_back(snapshot);
	}

	return analytics;
}
} // namespace

int main()
{
	try
	{
		const std::filesystem::path assetsRoot = std::filesystem::current_path() / "assets";
		const std::filesystem::path outputDirectory =
			"C:/Users/antllcon/AppData/Local/Temp/claude/D--parley/9cae2402-1135-4cdc-9d7a-e10cc4fa5db6/scratchpad";

		const auto polzaClient = std::make_shared<PolzaClient>(
			"http://127.0.0.1:1",
			"fixture-key",
			"fixture-model");
		const auto narrator = std::make_shared<OnePagerNarrator>(polzaClient);

		MetricFormatOptions formatOptions;

		const OnePagerDocumentBuilder builder(
			narrator,
			outputDirectory,
			CreateStrategyPartnersTheme(assetsRoot),
			formatOptions);

		const CompanyAnalytics analytics = BuildFixture();

		DocumentBuildOptions withNames;
		withNames.anonymize = false;
		withNames.showSourceNotes = true;
		withNames.author = "Investment Analysis";

		DocumentBuildOptions anonymized = withNames;
		anonymized.anonymize = true;

		const auto resultWithNames = builder.Build(analytics, withNames);
		std::cout << "Non-anonymized: " << resultWithNames.path.string() << std::endl;

		const auto resultAnonymized = builder.Build(analytics, anonymized);
		std::cout << "Anonymized: " << resultAnonymized.path.string() << std::endl;
	}
	catch (const std::exception& error)
	{
		std::cerr << "[Error] " << error.what() << std::endl;
		return 1;
	}

	return 0;
}
