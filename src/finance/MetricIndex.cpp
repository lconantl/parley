#include "MetricIndex.hpp"
#include "Metric.hpp"
#include <algorithm>

namespace
{

void CountOrigin(const MetricOrigin origin, MetricSummary& summary)
{
	switch (origin)
	{
	case MetricOrigin::Reported:
		++summary.reported;
		return;
	case MetricOrigin::Computed:
		++summary.computed;
		return;
	case MetricOrigin::Estimated:
		++summary.estimated;
		return;
	case MetricOrigin::NotApplicable:
		++summary.notApplicable;
		return;
	case MetricOrigin::Unavailable:
		++summary.missing;
		return;
	}
}
} // namespace

MetricIndex::MetricIndex(CompanyAnalytics& analytics)
{
	RegisterRevenue(analytics);
	RegisterProfit(analytics);
	RegisterMargins(analytics);
	RegisterCashFlow(analytics);
	RegisterWorkingCapital(analytics);
	RegisterDebt(analytics);
	RegisterBalance(analytics);
	RegisterLiquidity(analytics);
	RegisterReturns(analytics);
	RegisterInvestment(analytics);
	RegisterShareholders(analytics);
	RegisterValuation(analytics);
	RegisterQuality(analytics);
	RegisterMarket(analytics);
}

void MetricIndex::Register(
	const char* id,
	const char* title,
	const MetricUnit unit,
	MetricValue& value)
{
	m_descriptors.push_back({id, title, unit, m_group, &value});
}

void MetricIndex::OpenGroup(const MetricGroup group) noexcept
{
	m_group = group;
}

void MetricIndex::RegisterRevenue(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::Revenue);

	Register("revenue", "Выручка", MetricUnit::Money, analytics.revenue.revenue);
	Register("revenue_previous", "Выручка прошлого года", MetricUnit::Money, analytics.revenue.revenuePrevious);
	Register("revenue_growth", "Рост выручки", MetricUnit::Percent, analytics.revenue.revenueGrowth);
	Register("revenue_cagr", "Среднегодовой рост выручки за 3 года", MetricUnit::Percent, analytics.revenue.revenueCagr);
}

void MetricIndex::RegisterProfit(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::Profit);

	Register("cost_of_sales", "Себестоимость", MetricUnit::Money, analytics.profit.costOfSales);
	Register("gross_profit", "Валовая прибыль", MetricUnit::Money, analytics.profit.grossProfit);
	Register("selling_expenses", "Коммерческие расходы", MetricUnit::Money, analytics.profit.sellingExpenses);
	Register("administrative_expenses", "Управленческие расходы", MetricUnit::Money, analytics.profit.administrativeExpenses);
	Register("operating_expenses", "Операционные расходы", MetricUnit::Money, analytics.profit.operatingExpenses);
	Register("ebit", "EBIT", MetricUnit::Money, analytics.profit.ebit);
	Register("depreciation", "Амортизация", MetricUnit::Money, analytics.profit.depreciation);
	Register("ebitda", "EBITDA", MetricUnit::Money, analytics.profit.ebitda);
	Register("interest_expense", "Проценты к уплате", MetricUnit::Money, analytics.profit.interestExpense);
	Register("profit_before_tax", "Прибыль до налогообложения", MetricUnit::Money, analytics.profit.profitBeforeTax);
	Register("income_tax", "Налог на прибыль", MetricUnit::Money, analytics.profit.incomeTax);
	Register("effective_tax_rate", "Эффективная ставка налога", MetricUnit::Percent, analytics.profit.effectiveTaxRate);
	Register("net_profit", "Чистая прибыль", MetricUnit::Money, analytics.profit.netProfit);
}

void MetricIndex::RegisterMargins(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::Margins);

	Register("gross_margin", "Валовая маржа", MetricUnit::Percent, analytics.margins.grossMargin);
	Register("operating_margin", "Операционная маржа", MetricUnit::Percent, analytics.margins.operatingMargin);
	Register("ebitda_margin", "Маржа по EBITDA", MetricUnit::Percent, analytics.margins.ebitdaMargin);
	Register("net_margin", "Чистая маржа", MetricUnit::Percent, analytics.margins.netMargin);
	Register("fcf_margin", "Маржа по свободному денежному потоку", MetricUnit::Percent, analytics.margins.freeCashFlowMargin);
}

void MetricIndex::RegisterCashFlow(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::CashFlow);

	Register("operating_cash_flow", "Операционный денежный поток", MetricUnit::Money, analytics.cashFlow.operatingCashFlow);
	Register("investing_cash_flow", "Инвестиционный денежный поток", MetricUnit::Money, analytics.cashFlow.investingCashFlow);
	Register("financing_cash_flow", "Финансовый денежный поток", MetricUnit::Money, analytics.cashFlow.financingCashFlow);
	Register("net_cash_flow", "Чистый денежный поток", MetricUnit::Money, analytics.cashFlow.netCashFlow);
	Register("capex", "Капитальные затраты", MetricUnit::Money, analytics.cashFlow.capitalExpenditure);
	Register("free_cash_flow", "Свободный денежный поток", MetricUnit::Money, analytics.cashFlow.freeCashFlow);
	Register("cash", "Денежные средства", MetricUnit::Money, analytics.cashFlow.cash);
	Register("cash_conversion", "Конверсия прибыли в денежный поток", MetricUnit::Ratio, analytics.cashFlow.cashConversion);
}

void MetricIndex::RegisterWorkingCapital(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::WorkingCapital);

	Register("inventory", "Запасы", MetricUnit::Money, analytics.workingCapital.inventory);
	Register("receivables", "Дебиторская задолженность", MetricUnit::Money, analytics.workingCapital.receivables);
	Register("payables", "Кредиторская задолженность", MetricUnit::Money, analytics.workingCapital.payables);
	Register("working_capital", "Оборотный капитал", MetricUnit::Money, analytics.workingCapital.workingCapital);
	Register("inventory_days", "Оборачиваемость запасов", MetricUnit::Days, analytics.workingCapital.inventoryDays);
	Register("receivable_days", "Оборачиваемость дебиторки", MetricUnit::Days, analytics.workingCapital.receivableDays);
	Register("payable_days", "Оборачиваемость кредиторки", MetricUnit::Days, analytics.workingCapital.payableDays);
	Register("cash_cycle", "Денежный цикл", MetricUnit::Days, analytics.workingCapital.cashCycle);
}

void MetricIndex::RegisterDebt(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::Debt);

	Register("short_term_debt", "Краткосрочный долг", MetricUnit::Money, analytics.debt.shortTermDebt);
	Register("long_term_debt", "Долгосрочный долг", MetricUnit::Money, analytics.debt.longTermDebt);
	Register("total_debt", "Совокупный долг", MetricUnit::Money, analytics.debt.totalDebt);
	Register("net_debt", "Чистый долг", MetricUnit::Money, analytics.debt.netDebt);
	Register("net_debt_to_ebitda", "Чистый долг к EBITDA", MetricUnit::Ratio, analytics.debt.netDebtToEbitda);
	Register("interest_coverage", "Покрытие процентов", MetricUnit::Ratio, analytics.debt.interestCoverage);
	Register("debt_to_equity", "Долг к собственному капиталу", MetricUnit::Ratio, analytics.debt.debtToEquity);
}

void MetricIndex::RegisterBalance(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::Balance);

	Register("total_assets", "Активы", MetricUnit::Money, analytics.balance.totalAssets);
	Register("total_liabilities", "Пассивы", MetricUnit::Money, analytics.balance.totalLiabilities);
	Register("equity", "Собственный капитал", MetricUnit::Money, analytics.balance.equity);
	Register("non_current_assets", "Внеоборотные активы", MetricUnit::Money, analytics.balance.nonCurrentAssets);
	Register("current_assets", "Оборотные активы", MetricUnit::Money, analytics.balance.currentAssets);
	Register("current_liabilities", "Краткосрочные обязательства", MetricUnit::Money, analytics.balance.currentLiabilities);
	Register("fixed_assets", "Основные средства", MetricUnit::Money, analytics.balance.fixedAssets);
	Register("intangible_assets", "Нематериальные активы", MetricUnit::Money, analytics.balance.intangibleAssets);
	Register("retained_earnings", "Нераспределенная прибыль", MetricUnit::Money, analytics.balance.retainedEarnings);
}

void MetricIndex::RegisterLiquidity(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::Liquidity);

	Register("current_ratio", "Текущая ликвидность", MetricUnit::Ratio, analytics.liquidity.currentRatio);
	Register("quick_ratio", "Быстрая ликвидность", MetricUnit::Ratio, analytics.liquidity.quickRatio);
	Register("absolute_ratio", "Абсолютная ликвидность", MetricUnit::Ratio, analytics.liquidity.absoluteRatio);
	Register("equity_ratio", "Коэффициент автономии", MetricUnit::Percent, analytics.liquidity.equityRatio);
	Register("altman_score", "Z-счет Альтмана", MetricUnit::Ratio, analytics.liquidity.altmanScore);
}

void MetricIndex::RegisterReturns(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::Returns);

	Register("nopat", "NOPAT", MetricUnit::Money, analytics.returns.nopat);
	Register("invested_capital", "Инвестированный капитал", MetricUnit::Money, analytics.returns.investedCapital);
	Register("roe", "ROE", MetricUnit::Percent, analytics.returns.returnOnEquity);
	Register("roa", "ROA", MetricUnit::Percent, analytics.returns.returnOnAssets);
	Register("roic", "ROIC", MetricUnit::Percent, analytics.returns.returnOnInvestedCapital);
	Register("roce", "ROCE", MetricUnit::Percent, analytics.returns.returnOnCapitalEmployed);
	Register("asset_turnover", "Оборачиваемость активов", MetricUnit::Ratio, analytics.returns.assetTurnover);
}

void MetricIndex::RegisterInvestment(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::Investment);

	Register("capex_to_revenue", "CAPEX к выручке", MetricUnit::Percent, analytics.investment.capexToRevenue);
	Register("research_assets", "Капитализированные НИОКР", MetricUnit::Money, analytics.investment.researchAssets);
	Register("research_expense", "Расходы на исследования и разработки", MetricUnit::Money, analytics.investment.researchExpense);
	Register("research_to_revenue", "НИОКР к выручке", MetricUnit::Percent, analytics.investment.researchToRevenue);
}

void MetricIndex::RegisterShareholders(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::Shareholders);

	Register("dividends_paid", "Выплаченные дивиденды", MetricUnit::Money, analytics.shareholders.dividendsPaid);
	Register("share_buyback", "Выкуп собственных акций и долей", MetricUnit::Money, analytics.shareholders.shareBuyback);
	Register("share_count", "Количество акций", MetricUnit::Count, analytics.shareholders.shareCount);
	Register("eps", "Прибыль на акцию", MetricUnit::Money, analytics.shareholders.earningsPerShare);
	Register("dividend_per_share", "Дивиденд на акцию", MetricUnit::Money, analytics.shareholders.dividendPerShare);
	Register("book_value_per_share", "Балансовая стоимость на акцию", MetricUnit::Money, analytics.shareholders.bookValuePerShare);
	Register("payout_ratio", "Доля прибыли на дивиденды", MetricUnit::Percent, analytics.shareholders.payoutRatio);
}

void MetricIndex::RegisterValuation(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::Valuation);

	Register("market_capitalization", "Оценка стоимости бизнеса", MetricUnit::Money, analytics.valuation.marketCapitalization);
	Register("enterprise_value", "Стоимость предприятия", MetricUnit::Money, analytics.valuation.enterpriseValue);
	Register("price_to_earnings", "P/E", MetricUnit::Ratio, analytics.valuation.priceToEarnings);
	Register("ev_to_ebitda", "EV/EBITDA", MetricUnit::Ratio, analytics.valuation.enterpriseToEbitda);
	Register("ev_to_revenue", "EV/Revenue", MetricUnit::Ratio, analytics.valuation.enterpriseToRevenue);
	Register("price_to_fcf", "P/FCF", MetricUnit::Ratio, analytics.valuation.priceToFreeCashFlow);
	Register("price_to_book", "P/B", MetricUnit::Ratio, analytics.valuation.priceToBook);
}

void MetricIndex::RegisterQuality(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::Quality);

	Register("one_off_income", "Разовые доходы", MetricUnit::Money, analytics.quality.oneOffIncome);
	Register("one_off_expense", "Разовые расходы", MetricUnit::Money, analytics.quality.oneOffExpense);
	Register("normalized_net_profit", "Нормализованная чистая прибыль", MetricUnit::Money, analytics.quality.normalizedNetProfit);
	Register("accrual_ratio", "Начисленческая составляющая прибыли", MetricUnit::Percent, analytics.quality.accrualRatio);
}

void MetricIndex::RegisterMarket(CompanyAnalytics& analytics)
{
	OpenGroup(MetricGroup::Market);

	Register("customer_count", "Количество клиентов", MetricUnit::Count, analytics.customers.count);
	Register("top_client_share", "Доля крупнейшего клиента", MetricUnit::Percent, analytics.customers.topClientShare);
	Register("top_five_share", "Доля топ-5 клиентов", MetricUnit::Percent, analytics.customers.topFiveShare);
	Register("customer_retention", "Удержание клиентов", MetricUnit::Percent, analytics.customers.retention);
	Register("average_check", "Средний чек", MetricUnit::Money, analytics.customers.averageCheck);
	Register("market_size", "Размер рынка", MetricUnit::Money, analytics.market.size);
	Register("market_growth", "Рост рынка", MetricUnit::Percent, analytics.market.growth);
	Register("market_share", "Доля рынка", MetricUnit::Percent, analytics.market.share);
}

const std::vector<MetricDescriptor>& MetricIndex::ListAll() const noexcept
{
	return m_descriptors;
}

std::vector<MetricDescriptor> MetricIndex::ListMissing() const
{
	std::vector<MetricDescriptor> missing;

	for (const auto& descriptor : m_descriptors)
	{
		if (Metric::IsMissing(*descriptor.value))
		{
			missing.push_back(descriptor);
		}
	}

	return missing;
}

MetricSummary MetricIndex::Summarize() const
{
	MetricSummary summary;
	summary.total = m_descriptors.size();

	for (const auto& descriptor : m_descriptors)
	{
		CountOrigin(descriptor.value->origin, summary);
	}

	return summary;
}

AnalyticsStatus MetricIndex::EvaluateStatus() const
{
	const MetricSummary summary = Summarize();
	const std::size_t factual = summary.reported + summary.computed;

	if (factual == 0 && summary.estimated == 0)
	{
		return AnalyticsStatus::NoData;
	}

	if (factual == 0)
	{
		return AnalyticsStatus::EstimatedOnly;
	}

	if (summary.missing == 0)
	{
		return AnalyticsStatus::Complete;
	}

	return AnalyticsStatus::Partial;
}

bool MetricIndex::Apply(const std::string& id, const MetricValue& value)
{
	const auto iterator = std::find_if(
		m_descriptors.begin(),
		m_descriptors.end(),
		[&id](const MetricDescriptor& descriptor) {
			return descriptor.id == id;
		});

	if (iterator == m_descriptors.end())
	{
		return false;
	}

	if (!Metric::IsMissing(*iterator->value))
	{
		return false;
	}

	*iterator->value = value;

	return true;
}
