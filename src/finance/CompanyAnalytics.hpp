#pragma once

#include "MetricValue.hpp"

#include <string>
#include <vector>

enum class AnalyticsStatus
{
	NoData,
	EstimatedOnly,
	Partial,
	Complete
};

struct RevenueMetrics
{
	MetricValue revenue;
	MetricValue revenuePrevious;
	MetricValue revenueGrowth;
	MetricValue revenueCagr;
};

struct ProfitMetrics
{
	MetricValue costOfSales;
	MetricValue grossProfit;
	MetricValue sellingExpenses;
	MetricValue administrativeExpenses;
	MetricValue operatingExpenses;
	MetricValue ebit;
	MetricValue depreciation;
	MetricValue ebitda;
	MetricValue interestExpense;
	MetricValue profitBeforeTax;
	MetricValue incomeTax;
	MetricValue effectiveTaxRate;
	MetricValue netProfit;
};

struct MarginMetrics
{
	MetricValue grossMargin;
	MetricValue operatingMargin;
	MetricValue ebitdaMargin;
	MetricValue netMargin;
	MetricValue freeCashFlowMargin;
};

struct CashFlowMetrics
{
	MetricValue operatingCashFlow;
	MetricValue investingCashFlow;
	MetricValue financingCashFlow;
	MetricValue netCashFlow;
	MetricValue capitalExpenditure;
	MetricValue freeCashFlow;
	MetricValue cash;
	MetricValue cashConversion;
};

struct WorkingCapitalMetrics
{
	MetricValue inventory;
	MetricValue receivables;
	MetricValue payables;
	MetricValue workingCapital;
	MetricValue inventoryDays;
	MetricValue receivableDays;
	MetricValue payableDays;
	MetricValue cashCycle;
};

struct DebtMetrics
{
	MetricValue shortTermDebt;
	MetricValue longTermDebt;
	MetricValue totalDebt;
	MetricValue netDebt;
	MetricValue netDebtToEbitda;
	MetricValue interestCoverage;
	MetricValue debtToEquity;
};

struct BalanceMetrics
{
	MetricValue totalAssets;
	MetricValue totalLiabilities;
	MetricValue equity;
	MetricValue nonCurrentAssets;
	MetricValue currentAssets;
	MetricValue currentLiabilities;
	MetricValue fixedAssets;
	MetricValue intangibleAssets;
	MetricValue retainedEarnings;
};

struct LiquidityMetrics
{
	MetricValue currentRatio;
	MetricValue quickRatio;
	MetricValue absoluteRatio;
	MetricValue equityRatio;
	MetricValue altmanScore;
};

struct ReturnMetrics
{
	MetricValue nopat;
	MetricValue investedCapital;
	MetricValue returnOnEquity;
	MetricValue returnOnAssets;
	MetricValue returnOnInvestedCapital;
	MetricValue returnOnCapitalEmployed;
	MetricValue assetTurnover;
};

struct InvestmentMetrics
{
	MetricValue capexToRevenue;
	MetricValue researchAssets;
	MetricValue researchExpense;
	MetricValue researchToRevenue;
};

struct ShareholderMetrics
{
	MetricValue dividendsPaid;
	MetricValue shareBuyback;
	MetricValue shareCount;
	MetricValue earningsPerShare;
	MetricValue dividendPerShare;
	MetricValue bookValuePerShare;
	MetricValue payoutRatio;
};

struct ValuationMetrics
{
	MetricValue marketCapitalization;
	MetricValue enterpriseValue;
	MetricValue priceToEarnings;
	MetricValue enterpriseToEbitda;
	MetricValue enterpriseToRevenue;
	MetricValue priceToFreeCashFlow;
	MetricValue priceToBook;
};

struct QualityMetrics
{
	MetricValue oneOffIncome;
	MetricValue oneOffExpense;
	MetricValue normalizedNetProfit;
	MetricValue accrualRatio;
};

struct CustomerMetrics
{
	MetricValue count;
	MetricValue topClientShare;
	MetricValue topFiveShare;
	MetricValue retention;
	MetricValue averageCheck;
};

struct MarketMetrics
{
	MetricValue size;
	MetricValue growth;
	MetricValue share;
	std::string definition;
};

struct BusinessSegment
{
	std::string name;
	MetricValue revenue;
	MetricValue profit;
	MetricValue growth;
	MetricValue revenueShare;
};

struct Competitor
{
	std::string name;
	std::string inn;
	MetricValue revenue;
	MetricValue revenueGrowth;
	MetricValue margin;
	MetricValue valuation;
};

struct OperationalIndicator
{
	std::string name;
	std::string unit;
	MetricValue value;
};

struct LegalRisk
{
	std::string title;
	std::string description;
	std::string severity;
	std::string source;
};

struct AnnualSnapshot
{
	int year = 0;
	MetricValue revenue;
	MetricValue revenueGrowth;
	MetricValue grossProfit;
	MetricValue ebit;
	MetricValue ebitda;
	MetricValue netProfit;
	MetricValue operatingCashFlow;
	MetricValue freeCashFlow;
	MetricValue totalAssets;
	MetricValue equity;
	MetricValue totalDebt;
};

struct RelatedParty
{
	std::string name;
	std::string inn;
	std::string role;
	std::string share;
};

struct CompanyAnalytics
{
	std::string identifier;
	std::string name;
	std::string activity;
	std::string region;
	std::string legalForm;
	int year = 0;
	AnalyticsStatus status = AnalyticsStatus::NoData;

	RevenueMetrics revenue;
	ProfitMetrics profit;
	MarginMetrics margins;
	CashFlowMetrics cashFlow;
	WorkingCapitalMetrics workingCapital;
	DebtMetrics debt;
	BalanceMetrics balance;
	LiquidityMetrics liquidity;
	ReturnMetrics returns;
	InvestmentMetrics investment;
	ShareholderMetrics shareholders;
	ValuationMetrics valuation;
	QualityMetrics quality;
	CustomerMetrics customers;
	MarketMetrics market;

	std::vector<AnnualSnapshot> series;
	std::vector<BusinessSegment> segments;
	std::vector<Competitor> competitors;
	std::vector<OperationalIndicator> operations;
	std::vector<LegalRisk> legalRisks;
	std::vector<RelatedParty> relatedParties;

	std::vector<std::string> assumptions;
	std::string researchNotes;
	double estimationCost = 0.0;
};