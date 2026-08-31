#include "FinancialCalculator.hpp"

#include "Metric.hpp"
#include "ReportCodes.hpp"

#include <cmath>
#include <optional>
#include <string>
#include <utility>

namespace
{
constexpr double Epsilon = 1e-9;
constexpr double PercentScale = 100.0;
constexpr double DaysInYear = 365.0;
constexpr double DepreciationProxyConfidence = 0.35;
constexpr double NormalizationConfidence = 0.5;
constexpr double DefaultTaxRateBefore2025 = 0.20;
constexpr double DefaultTaxRateFrom2025 = 0.25;
constexpr int TaxReformYear = 2025;

class LineReader
{
public:
	LineReader(const FinancialHistory& history, const int year)
		: m_history(history)
		, m_year(year)
	{
	}

	std::optional<double> Value(const int code) const
	{
		return m_history.FindLine(m_year, code);
	}

	std::optional<double> Absolute(const int code) const
	{
		const std::optional<double> value = Value(code);
		if (!value.has_value())
		{
			return std::nullopt;
		}

		return std::abs(value.value());
	}

	std::optional<double> Previous(const int code) const
	{
		return m_history.FindLine(m_year - 1, code);
	}

	std::optional<double> Ago(const int code, const int years) const
	{
		return m_history.FindLine(m_year - years, code);
	}

private:
	const FinancialHistory& m_history;
	int m_year;
};

void FillReported(MetricValue& target, const std::optional<double>& value, std::string missing)
{
	if (value.has_value())
	{
		target = Metric::Reported(value.value());
		return;
	}

	target = Metric::Unavailable(std::move(missing));
}

bool IsFillable(const MetricValue& target)
{
	return Metric::IsMissing(target);
}

bool AreKnown(const MetricValue& first, const MetricValue& second)
{
	return Metric::IsKnown(first) && Metric::IsKnown(second);
}

void FillRatio(
	MetricValue& target,
	const MetricValue& numerator,
	const MetricValue& denominator,
	const double scale)
{
	if (!IsFillable(target) || !AreKnown(numerator, denominator))
	{
		return;
	}

	if (std::abs(denominator.value) < Epsilon)
	{
		return;
	}

	target = Metric::Computed(
		numerator.value / denominator.value * scale,
		Metric::CombineConfidence(numerator.confidence, denominator.confidence));
}

void FillCombination(
	MetricValue& target,
	const MetricValue& first,
	const MetricValue& second,
	const double secondSign)
{
	if (!IsFillable(target) || !AreKnown(first, second))
	{
		return;
	}

	target = Metric::Computed(
		first.value + second.value * secondSign,
		Metric::CombineConfidence(first.confidence, second.confidence));
}

double DefaultTaxRate(const int year)
{
	return year >= TaxReformYear ? DefaultTaxRateFrom2025 : DefaultTaxRateBefore2025;
}

bool IsShareBased(const CompanyProfile& profile)
{
	if (profile.registry.type == PartyType::Individual)
	{
		return false;
	}

	const std::string& form = profile.registry.opf;

	return form == "ПАО" || form == "АО" || form == "ОАО" || form == "ЗАО";
}

void ExtractIncomeStatement(const LineReader& reader, CompanyAnalytics& analytics)
{
	FillReported(analytics.revenue.revenue, reader.Value(ReportCodes::Revenue),
		"нужна строка 2110 отчета о финансовых результатах");
	FillReported(analytics.revenue.revenuePrevious, reader.Previous(ReportCodes::Revenue),
		"нужна выручка за предыдущий год");
	FillReported(analytics.profit.costOfSales, reader.Absolute(ReportCodes::CostOfSales),
		"нужна строка 2120 или валовая прибыль 2100");
	FillReported(analytics.profit.grossProfit, reader.Value(ReportCodes::GrossProfit),
		"нужна строка 2100 или себестоимость 2120");
	FillReported(analytics.profit.sellingExpenses, reader.Absolute(ReportCodes::SellingExpenses),
		"нужна строка 2210");
	FillReported(analytics.profit.administrativeExpenses,
		reader.Absolute(ReportCodes::AdministrativeExpenses), "нужна строка 2220");
	FillReported(analytics.profit.ebit, reader.Value(ReportCodes::OperatingProfit),
		"нужна строка 2200 или связка 2300 и 2330");
	FillReported(analytics.profit.interestExpense, reader.Absolute(ReportCodes::InterestExpense),
		"нужна строка 2330");
	FillReported(analytics.profit.profitBeforeTax, reader.Value(ReportCodes::ProfitBeforeTax),
		"нужна строка 2300");
	FillReported(analytics.profit.incomeTax, reader.Absolute(ReportCodes::IncomeTax),
		"нужна строка 2410");
	FillReported(analytics.profit.netProfit, reader.Value(ReportCodes::NetProfit),
		"нужна строка 2400");
	FillReported(analytics.quality.oneOffIncome, reader.Value(ReportCodes::OtherIncome),
		"нужна строка 2340");
	FillReported(analytics.quality.oneOffExpense, reader.Absolute(ReportCodes::OtherExpense),
		"нужна строка 2350");
}

void ExtractBalanceSheet(const LineReader& reader, CompanyAnalytics& analytics)
{
	FillReported(analytics.balance.totalAssets, reader.Value(ReportCodes::TotalAssets),
		"нужна строка 1600");
	FillReported(analytics.balance.totalLiabilities, reader.Value(ReportCodes::TotalLiabilities),
		"нужна строка 1700");
	FillReported(analytics.balance.equity, reader.Value(ReportCodes::Equity),
		"нужна строка 1300");
	FillReported(analytics.balance.nonCurrentAssets, reader.Value(ReportCodes::NonCurrentAssets),
		"нужна строка 1100");
	FillReported(analytics.balance.currentAssets, reader.Value(ReportCodes::CurrentAssets),
		"нужна строка 1200");
	FillReported(analytics.balance.currentLiabilities,
		reader.Value(ReportCodes::ShortTermLiabilities), "нужна строка 1500");
	FillReported(analytics.balance.fixedAssets, reader.Value(ReportCodes::FixedAssets),
		"нужна строка 1150");
	FillReported(analytics.balance.intangibleAssets, reader.Value(ReportCodes::IntangibleAssets),
		"нужна строка 1110");
	FillReported(analytics.balance.retainedEarnings, reader.Value(ReportCodes::RetainedEarnings),
		"нужна строка 1370");

	FillReported(analytics.workingCapital.inventory, reader.Value(ReportCodes::Inventory),
		"нужна строка 1210");
	FillReported(analytics.workingCapital.receivables, reader.Value(ReportCodes::Receivables),
		"нужна строка 1230");
	FillReported(analytics.workingCapital.payables, reader.Value(ReportCodes::Payables),
		"нужна строка 1520");

	FillReported(analytics.debt.longTermDebt, reader.Value(ReportCodes::LongTermDebt),
		"нужна строка 1410");
	FillReported(analytics.debt.shortTermDebt, reader.Value(ReportCodes::ShortTermDebt),
		"нужна строка 1510");

	FillReported(analytics.investment.researchAssets, reader.Value(ReportCodes::ResearchResults),
		"нужна строка 1120");
}

void ExtractCashFlow(const LineReader& reader, CompanyAnalytics& analytics)
{
	FillReported(analytics.cashFlow.operatingCashFlow,
		reader.Value(ReportCodes::OperatingCashFlow), "нужна строка 4100 отчета о движении денежных средств");
	FillReported(analytics.cashFlow.investingCashFlow,
		reader.Value(ReportCodes::InvestingCashFlow), "нужна строка 4200");
	FillReported(analytics.cashFlow.financingCashFlow,
		reader.Value(ReportCodes::FinancingCashFlow), "нужна строка 4300");
	FillReported(analytics.cashFlow.netCashFlow, reader.Value(ReportCodes::NetCashFlow),
		"нужна строка 4400");
	FillReported(analytics.cashFlow.capitalExpenditure,
		reader.Absolute(ReportCodes::CapitalExpenditure), "нужна строка 4221");
	FillReported(analytics.cashFlow.cash, reader.Value(ReportCodes::Cash),
		"нужна строка 1250");

	FillReported(analytics.shareholders.dividendsPaid, reader.Absolute(ReportCodes::DividendsPaid),
		"нужна строка 4322");
	FillReported(analytics.shareholders.shareBuyback, reader.Absolute(ReportCodes::ShareBuyback),
		"нужна строка 4321");
}

void ExtractDepreciationProxy(const LineReader& reader, CompanyAnalytics& analytics)
{
	analytics.profit.depreciation = Metric::Unavailable(
		"амортизация не раскрывается в формах РСБУ, нужна отраслевая оценка");

	const std::optional<double> capex = reader.Absolute(ReportCodes::CapitalExpenditure);
	const std::optional<double> current = reader.Value(ReportCodes::FixedAssets);
	const std::optional<double> previous = reader.Previous(ReportCodes::FixedAssets);

	if (!capex.has_value() || !current.has_value() || !previous.has_value())
	{
		return;
	}

	const double proxy = previous.value() + capex.value() - current.value();
	if (proxy <= 0.0)
	{
		return;
	}

	analytics.profit.depreciation = Metric::Estimated(
		proxy,
		DepreciationProxyConfidence,
		"приближение через изменение основных средств и CAPEX, без учета выбытий");
}

void ExtractRevenueTrend(const LineReader& reader, CompanyAnalytics& analytics)
{
	const std::optional<double> current = reader.Value(ReportCodes::Revenue);
	const std::optional<double> past = reader.Ago(ReportCodes::Revenue, 3);

	analytics.revenue.revenueCagr = Metric::Unavailable("нужна выручка за три предыдущих года");

	if (!current.has_value() || !past.has_value())
	{
		return;
	}

	if (past.value() < Epsilon || current.value() < 0.0)
	{
		return;
	}

	const double ratio = current.value() / past.value();
	const double cagr = std::pow(ratio, 1.0 / 3.0) - 1.0;

	analytics.revenue.revenueCagr = Metric::Computed(cagr * PercentScale, 1.0);
}

void MarkShareMetrics(const CompanyProfile& profile, CompanyAnalytics& analytics)
{
	if (IsShareBased(profile))
	{
		analytics.shareholders.shareCount = Metric::Unavailable(
			"количество акций не раскрывается в отчетности, нужен внешний источник");
		return;
	}

	const std::string reason = "у этой организационно-правовой формы нет акций, есть доли участников";

	analytics.shareholders.shareCount = Metric::NotApplicable(reason);
	analytics.shareholders.earningsPerShare = Metric::NotApplicable(reason);
	analytics.shareholders.dividendPerShare = Metric::NotApplicable(reason);
	analytics.shareholders.bookValuePerShare = Metric::NotApplicable(reason);
}

void FillIdentity(const CompanyProfile& profile, CompanyAnalytics& analytics)
{
	analytics.identifier = profile.registry.inn;
	analytics.name = profile.registry.shortName;
	analytics.region = profile.registry.address;
	analytics.legalForm = profile.registry.opf;

	for (const auto& activity : profile.registry.activities)
	{
		if (activity.main)
		{
			analytics.activity = activity.code + " " + activity.name;
			break;
		}
	}
}

void FillLegalRisks(const CompanyProfile& profile, CompanyAnalytics& analytics)
{
	for (const auto& description : profile.risks.descriptions)
	{
		analytics.legalRisks.push_back({description, {}, "high", "ЕГРЮЛ"});
	}
}

void FillRelatedParties(const CompanyProfile& profile, CompanyAnalytics& analytics)
{
	for (const auto& founder : profile.founders)
	{
		analytics.relatedParties.push_back({founder.name, founder.inn, "учредитель", {}});
	}

	for (const auto& manager : profile.employees.managers)
	{
		analytics.relatedParties.push_back({manager.name, manager.inn, manager.post, {}});
	}

	for (const auto& company : profile.affiliatedCompanies)
	{
		analytics.relatedParties.push_back({company.name, company.inn, "аффилированная компания", {}});
	}
}

void FillEmptyStatements(CompanyAnalytics& analytics)
{
	const std::string reason = "отчетность за выбранный год недоступна";

	analytics.revenue.revenue = Metric::Unavailable(reason);
	analytics.revenue.revenuePrevious = Metric::Unavailable(reason);
	analytics.revenue.revenueCagr = Metric::Unavailable(reason);
	analytics.profit.netProfit = Metric::Unavailable(reason);
	analytics.balance.totalAssets = Metric::Unavailable(reason);
	analytics.cashFlow.operatingCashFlow = Metric::Unavailable(reason);
	analytics.profit.depreciation = Metric::Unavailable(reason);
}

void DeriveRevenue(CompanyAnalytics& analytics)
{
	if (IsFillable(analytics.revenue.revenueGrowth)
		&& AreKnown(analytics.revenue.revenue, analytics.revenue.revenuePrevious)
		&& std::abs(analytics.revenue.revenuePrevious.value) > Epsilon)
	{
		const double growth = analytics.revenue.revenue.value
				/ analytics.revenue.revenuePrevious.value
			- 1.0;

		analytics.revenue.revenueGrowth = Metric::Computed(
			growth * PercentScale,
			Metric::CombineConfidence(
				analytics.revenue.revenue.confidence,
				analytics.revenue.revenuePrevious.confidence));
	}
}

void DeriveProfit(CompanyAnalytics& analytics)
{
	FillCombination(analytics.profit.grossProfit, analytics.revenue.revenue,
		analytics.profit.costOfSales, -1.0);
	FillCombination(analytics.profit.costOfSales, analytics.revenue.revenue,
		analytics.profit.grossProfit, -1.0);
	FillCombination(analytics.profit.operatingExpenses, analytics.profit.sellingExpenses,
		analytics.profit.administrativeExpenses, 1.0);
	FillCombination(analytics.profit.operatingExpenses, analytics.profit.grossProfit,
		analytics.profit.ebit, -1.0);
	FillCombination(analytics.profit.ebit, analytics.profit.grossProfit,
		analytics.profit.operatingExpenses, -1.0);
	FillCombination(analytics.profit.ebit, analytics.profit.profitBeforeTax,
		analytics.profit.interestExpense, 1.0);
	FillCombination(analytics.profit.ebitda, analytics.profit.ebit,
		analytics.profit.depreciation, 1.0);
	FillCombination(analytics.profit.netProfit, analytics.profit.profitBeforeTax,
		analytics.profit.incomeTax, -1.0);

	FillRatio(analytics.profit.effectiveTaxRate, analytics.profit.incomeTax,
		analytics.profit.profitBeforeTax, PercentScale);
}

void DeriveMargins(CompanyAnalytics& analytics)
{
	const MetricValue& revenue = analytics.revenue.revenue;

	FillRatio(analytics.margins.grossMargin, analytics.profit.grossProfit, revenue, PercentScale);
	FillRatio(analytics.margins.operatingMargin, analytics.profit.ebit, revenue, PercentScale);
	FillRatio(analytics.margins.ebitdaMargin, analytics.profit.ebitda, revenue, PercentScale);
	FillRatio(analytics.margins.netMargin, analytics.profit.netProfit, revenue, PercentScale);
	FillRatio(analytics.margins.freeCashFlowMargin, analytics.cashFlow.freeCashFlow, revenue,
		PercentScale);
}

void DeriveCashFlow(CompanyAnalytics& analytics)
{
	FillCombination(analytics.cashFlow.freeCashFlow, analytics.cashFlow.operatingCashFlow,
		analytics.cashFlow.capitalExpenditure, -1.0);

	FillRatio(analytics.cashFlow.cashConversion, analytics.cashFlow.operatingCashFlow,
		analytics.profit.netProfit, 1.0);
}

void DeriveWorkingCapital(CompanyAnalytics& analytics)
{
	if (IsFillable(analytics.workingCapital.workingCapital)
		&& Metric::IsKnown(analytics.workingCapital.inventory)
		&& Metric::IsKnown(analytics.workingCapital.receivables)
		&& Metric::IsKnown(analytics.workingCapital.payables))
	{
		const double value = analytics.workingCapital.inventory.value
			+ analytics.workingCapital.receivables.value
			- analytics.workingCapital.payables.value;

		analytics.workingCapital.workingCapital = Metric::Computed(
			value,
			Metric::CombineConfidence(
				analytics.workingCapital.inventory.confidence,
				analytics.workingCapital.receivables.confidence,
				analytics.workingCapital.payables.confidence));
	}

	FillRatio(analytics.workingCapital.inventoryDays, analytics.workingCapital.inventory,
		analytics.profit.costOfSales, DaysInYear);
	FillRatio(analytics.workingCapital.receivableDays, analytics.workingCapital.receivables,
		analytics.revenue.revenue, DaysInYear);
	FillRatio(analytics.workingCapital.payableDays, analytics.workingCapital.payables,
		analytics.profit.costOfSales, DaysInYear);

	if (IsFillable(analytics.workingCapital.cashCycle)
		&& Metric::IsKnown(analytics.workingCapital.inventoryDays)
		&& Metric::IsKnown(analytics.workingCapital.receivableDays)
		&& Metric::IsKnown(analytics.workingCapital.payableDays))
	{
		const double cycle = analytics.workingCapital.inventoryDays.value
			+ analytics.workingCapital.receivableDays.value
			- analytics.workingCapital.payableDays.value;

		analytics.workingCapital.cashCycle = Metric::Computed(
			cycle,
			Metric::CombineConfidence(
				analytics.workingCapital.inventoryDays.confidence,
				analytics.workingCapital.receivableDays.confidence,
				analytics.workingCapital.payableDays.confidence));
	}
}

void DeriveDebt(CompanyAnalytics& analytics)
{
	FillCombination(analytics.debt.totalDebt, analytics.debt.longTermDebt,
		analytics.debt.shortTermDebt, 1.0);
	FillCombination(analytics.debt.netDebt, analytics.debt.totalDebt, analytics.cashFlow.cash,
		-1.0);

	FillRatio(analytics.debt.netDebtToEbitda, analytics.debt.netDebt, analytics.profit.ebitda,
		1.0);
	FillRatio(analytics.debt.interestCoverage, analytics.profit.ebit,
		analytics.profit.interestExpense, 1.0);
	FillRatio(analytics.debt.debtToEquity, analytics.debt.totalDebt, analytics.balance.equity,
		1.0);
}

void DeriveLiquidity(CompanyAnalytics& analytics)
{
	FillRatio(analytics.liquidity.currentRatio, analytics.balance.currentAssets,
		analytics.balance.currentLiabilities, 1.0);
	FillRatio(analytics.liquidity.absoluteRatio, analytics.cashFlow.cash,
		analytics.balance.currentLiabilities, 1.0);
	FillRatio(analytics.liquidity.equityRatio, analytics.balance.equity,
		analytics.balance.totalLiabilities, PercentScale);

	if (IsFillable(analytics.liquidity.quickRatio)
		&& Metric::IsKnown(analytics.workingCapital.receivables)
		&& Metric::IsKnown(analytics.cashFlow.cash)
		&& Metric::IsKnown(analytics.balance.currentLiabilities)
		&& std::abs(analytics.balance.currentLiabilities.value) > Epsilon)
	{
		const double liquid = analytics.workingCapital.receivables.value
			+ analytics.cashFlow.cash.value;

		analytics.liquidity.quickRatio = Metric::Computed(
			liquid / analytics.balance.currentLiabilities.value,
			Metric::CombineConfidence(
				analytics.workingCapital.receivables.confidence,
				analytics.cashFlow.cash.confidence,
				analytics.balance.currentLiabilities.confidence));
	}
}

bool CanComputeAltman(const CompanyAnalytics& analytics)
{
	return Metric::IsKnown(analytics.balance.totalAssets)
		&& Metric::IsKnown(analytics.balance.currentAssets)
		&& Metric::IsKnown(analytics.balance.currentLiabilities)
		&& Metric::IsKnown(analytics.balance.retainedEarnings)
		&& Metric::IsKnown(analytics.balance.equity)
		&& Metric::IsKnown(analytics.profit.ebit)
		&& std::abs(analytics.balance.totalAssets.value) > Epsilon;
}

void DeriveAltman(CompanyAnalytics& analytics)
{
	if (!IsFillable(analytics.liquidity.altmanScore) || !CanComputeAltman(analytics))
	{
		return;
	}

	const double assets = analytics.balance.totalAssets.value;
	const double liabilities = assets - analytics.balance.equity.value;

	if (std::abs(liabilities) < Epsilon)
	{
		return;
	}

	const double workingCapitalRatio = (analytics.balance.currentAssets.value
										   - analytics.balance.currentLiabilities.value)
		/ assets;
	const double retainedRatio = analytics.balance.retainedEarnings.value / assets;
	const double operatingRatio = analytics.profit.ebit.value / assets;
	const double equityRatio = analytics.balance.equity.value / liabilities;

	const double score = 6.56 * workingCapitalRatio
		+ 3.26 * retainedRatio
		+ 6.72 * operatingRatio
		+ 1.05 * equityRatio;

	analytics.liquidity.altmanScore = Metric::Computed(
		score,
		Metric::CombineConfidence(
			analytics.balance.totalAssets.confidence,
			analytics.profit.ebit.confidence,
			analytics.balance.retainedEarnings.confidence));
}

void DeriveReturns(CompanyAnalytics& analytics)
{
	if (IsFillable(analytics.returns.nopat) && Metric::IsKnown(analytics.profit.ebit))
	{
		const bool hasRate = Metric::IsKnown(analytics.profit.effectiveTaxRate);
		const double rate = hasRate
			? analytics.profit.effectiveTaxRate.value / PercentScale
			: DefaultTaxRate(analytics.year);

		analytics.returns.nopat = Metric::Computed(
			analytics.profit.ebit.value * (1.0 - rate),
			hasRate ? analytics.profit.ebit.confidence
					: Metric::CombineConfidence(analytics.profit.ebit.confidence, 0.7));
	}

	if (IsFillable(analytics.returns.investedCapital)
		&& Metric::IsKnown(analytics.balance.equity)
		&& Metric::IsKnown(analytics.debt.totalDebt)
		&& Metric::IsKnown(analytics.cashFlow.cash))
	{
		const double capital = analytics.balance.equity.value
			+ analytics.debt.totalDebt.value
			- analytics.cashFlow.cash.value;

		analytics.returns.investedCapital = Metric::Computed(
			capital,
			Metric::CombineConfidence(
				analytics.balance.equity.confidence,
				analytics.debt.totalDebt.confidence,
				analytics.cashFlow.cash.confidence));
	}

	FillRatio(analytics.returns.returnOnEquity, analytics.profit.netProfit,
		analytics.balance.equity, PercentScale);
	FillRatio(analytics.returns.returnOnAssets, analytics.profit.netProfit,
		analytics.balance.totalAssets, PercentScale);
	FillRatio(analytics.returns.returnOnInvestedCapital, analytics.returns.nopat,
		analytics.returns.investedCapital, PercentScale);
	FillRatio(analytics.returns.assetTurnover, analytics.revenue.revenue,
		analytics.balance.totalAssets, 1.0);

	if (IsFillable(analytics.returns.returnOnCapitalEmployed)
		&& Metric::IsKnown(analytics.profit.ebit)
		&& Metric::IsKnown(analytics.balance.totalAssets)
		&& Metric::IsKnown(analytics.balance.currentLiabilities))
	{
		const double employed = analytics.balance.totalAssets.value
			- analytics.balance.currentLiabilities.value;

		if (std::abs(employed) > Epsilon)
		{
			analytics.returns.returnOnCapitalEmployed = Metric::Computed(
				analytics.profit.ebit.value / employed * PercentScale,
				Metric::CombineConfidence(
					analytics.profit.ebit.confidence,
					analytics.balance.totalAssets.confidence,
					analytics.balance.currentLiabilities.confidence));
		}
	}
}

void DeriveInvestment(CompanyAnalytics& analytics)
{
	FillRatio(analytics.investment.capexToRevenue, analytics.cashFlow.capitalExpenditure,
		analytics.revenue.revenue, PercentScale);
	FillRatio(analytics.investment.researchToRevenue, analytics.investment.researchExpense,
		analytics.revenue.revenue, PercentScale);
}

void DeriveShareholders(CompanyAnalytics& analytics)
{
	FillRatio(analytics.shareholders.earningsPerShare, analytics.profit.netProfit,
		analytics.shareholders.shareCount, 1.0);
	FillRatio(analytics.shareholders.dividendPerShare, analytics.shareholders.dividendsPaid,
		analytics.shareholders.shareCount, 1.0);
	FillRatio(analytics.shareholders.bookValuePerShare, analytics.balance.equity,
		analytics.shareholders.shareCount, 1.0);
	FillRatio(analytics.shareholders.payoutRatio, analytics.shareholders.dividendsPaid,
		analytics.profit.netProfit, PercentScale);
}

void DeriveValuation(CompanyAnalytics& analytics)
{
	FillCombination(analytics.valuation.enterpriseValue,
		analytics.valuation.marketCapitalization, analytics.debt.netDebt, 1.0);

	FillRatio(analytics.valuation.priceToEarnings, analytics.valuation.marketCapitalization,
		analytics.profit.netProfit, 1.0);
	FillRatio(analytics.valuation.priceToFreeCashFlow, analytics.valuation.marketCapitalization,
		analytics.cashFlow.freeCashFlow, 1.0);
	FillRatio(analytics.valuation.priceToBook, analytics.valuation.marketCapitalization,
		analytics.balance.equity, 1.0);
	FillRatio(analytics.valuation.enterpriseToEbitda, analytics.valuation.enterpriseValue,
		analytics.profit.ebitda, 1.0);
	FillRatio(analytics.valuation.enterpriseToRevenue, analytics.valuation.enterpriseValue,
		analytics.revenue.revenue, 1.0);
}

void DeriveQuality(CompanyAnalytics& analytics)
{
	if (IsFillable(analytics.quality.normalizedNetProfit)
		&& Metric::IsKnown(analytics.profit.netProfit)
		&& Metric::IsKnown(analytics.quality.oneOffIncome)
		&& Metric::IsKnown(analytics.quality.oneOffExpense))
	{
		const bool hasRate = Metric::IsKnown(analytics.profit.effectiveTaxRate);
		const double rate = hasRate
			? analytics.profit.effectiveTaxRate.value / PercentScale
			: DefaultTaxRate(analytics.year);

		const double oneOff = analytics.quality.oneOffIncome.value
			- analytics.quality.oneOffExpense.value;

		analytics.quality.normalizedNetProfit = Metric::Computed(
			analytics.profit.netProfit.value - oneOff * (1.0 - rate),
			Metric::CombineConfidence(
				analytics.profit.netProfit.confidence,
				NormalizationConfidence));
	}

	if (IsFillable(analytics.quality.accrualRatio)
		&& Metric::IsKnown(analytics.profit.netProfit)
		&& Metric::IsKnown(analytics.cashFlow.operatingCashFlow)
		&& Metric::IsKnown(analytics.balance.totalAssets)
		&& std::abs(analytics.balance.totalAssets.value) > Epsilon)
	{
		const double accrual = analytics.profit.netProfit.value
			- analytics.cashFlow.operatingCashFlow.value;

		analytics.quality.accrualRatio = Metric::Computed(
			accrual / analytics.balance.totalAssets.value * PercentScale,
			Metric::CombineConfidence(
				analytics.profit.netProfit.confidence,
				analytics.cashFlow.operatingCashFlow.confidence,
				analytics.balance.totalAssets.confidence));
	}
}

void DeriveSegments(CompanyAnalytics& analytics)
{
	for (auto& segment : analytics.segments)
	{
		FillRatio(segment.revenueShare, segment.revenue, analytics.revenue.revenue, PercentScale);
	}
}

AnnualSnapshot BuildSnapshot(const FinancialHistory& history, const int year)
{
	CompanyAnalytics yearly;
	yearly.year = year;

	const LineReader reader(history, year);

	ExtractIncomeStatement(reader, yearly);
	ExtractBalanceSheet(reader, yearly);
	ExtractCashFlow(reader, yearly);
	ExtractDepreciationProxy(reader, yearly);

	DeriveRevenue(yearly);
	DeriveProfit(yearly);
	DeriveCashFlow(yearly);
	DeriveDebt(yearly);

	AnnualSnapshot snapshot;
	snapshot.year = year;
	snapshot.revenue = yearly.revenue.revenue;
	snapshot.revenueGrowth = yearly.revenue.revenueGrowth;
	snapshot.grossProfit = yearly.profit.grossProfit;
	snapshot.ebit = yearly.profit.ebit;
	snapshot.ebitda = yearly.profit.ebitda;
	snapshot.netProfit = yearly.profit.netProfit;
	snapshot.operatingCashFlow = yearly.cashFlow.operatingCashFlow;
	snapshot.freeCashFlow = yearly.cashFlow.freeCashFlow;
	snapshot.totalAssets = yearly.balance.totalAssets;
	snapshot.equity = yearly.balance.equity;
	snapshot.totalDebt = yearly.debt.totalDebt;

	return snapshot;
}

void FillSeries(const FinancialHistory& history, CompanyAnalytics& analytics)
{
	for (const int year : history.GetYears())
	{
		if (year > analytics.year)
		{
			continue;
		}

		analytics.series.push_back(BuildSnapshot(history, year));
	}
}
} // namespace

CompanyAnalytics FinancialCalculator::Extract(
	const FinancialHistory& history,
	const CompanyProfile& profile,
	const int year)
{
	CompanyAnalytics analytics;
	analytics.year = year;

	FillIdentity(profile, analytics);
	FillLegalRisks(profile, analytics);
	FillRelatedParties(profile, analytics);
	MarkShareMetrics(profile, analytics);

	if (!history.HasYear(year))
	{
		FillEmptyStatements(analytics);
		FillSeries(history, analytics);
		return analytics;
	}

	const LineReader reader(history, year);

	ExtractIncomeStatement(reader, analytics);
	ExtractBalanceSheet(reader, analytics);
	ExtractCashFlow(reader, analytics);
	ExtractDepreciationProxy(reader, analytics);
	ExtractRevenueTrend(reader, analytics);
	FillSeries(history, analytics);

	return analytics;
}

void FinancialCalculator::Derive(CompanyAnalytics& analytics)
{
	DeriveRevenue(analytics);
	DeriveProfit(analytics);
	DeriveCashFlow(analytics);
	DeriveWorkingCapital(analytics);
	DeriveDebt(analytics);
	DeriveMargins(analytics);
	DeriveLiquidity(analytics);
	DeriveAltman(analytics);
	DeriveReturns(analytics);
	DeriveInvestment(analytics);
	DeriveShareholders(analytics);
	DeriveValuation(analytics);
	DeriveQuality(analytics);
	DeriveSegments(analytics);
}