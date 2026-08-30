#pragma once

#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

struct Margins
{
	double gross;
	double operating;
	double net;
	double ebitda;
};

struct IncomeStatement
{
	double revenue;
	double revenueGrowth;
	double costOfGoodsSold;
	double grossProfit;
	double operatingExpenses;
	double ebitda;
	double ebit;
	double netProfit;
	Margins margins;
	double researchAndDevelopment;
};

struct CashFlow
{
	double operatingCashFlow;
	double freeCashFlow;
	double capitalExpenditures;
	double dividends;
	double shareBuybacks;
};

struct BalanceSheet
{
	double cash;
	double accountsReceivable;
	double accountsPayable;
	double inventory;
	double workingCapital;
	double debt;
	double netDebt;
	double assets;
	double liabilities;
	double equity;
	double fixedAssets;
};

struct Efficiency
{
	double liquidity;
	double solvency;
	double roe;
	double roa;
	double roic;
};

struct Valuation
{
	double sharesOutstanding;
	double earningsPerShare;
	double priceToEarnings;
	double evToEbitda;
	double evToRevenue;
	double priceToFreeCashFlow;
};

struct BusinessSegment
{
	std::string name;
	double revenue;
	double profit;
	double growth;
};

struct ClientBase
{
	int count;
	double concentration;
	double retentionRate;
	double averageTicket;
};

struct MarketPosition
{
	double marketSize;
	double marketGrowth;
	double marketShare;
};

struct Competitor
{
	std::string name;
	double revenue;
	double growth;
	double margin;
	double valuation;
};

struct EarningsQuality
{
	double oneTimeIncome;
	double oneTimeExpenses;
	double normalizedProfit;
};

struct CompanyMetrics
{
	IncomeStatement incomeStatement;
	CashFlow cashFlow;
	BalanceSheet balanceSheet;
	Efficiency efficiency;
	Valuation valuation;
	std::vector<BusinessSegment> segments;
	ClientBase clients;
	MarketPosition market;
	std::vector<Competitor> competitors;
	std::vector<std::string> legalAndRegulatoryRisks;
	std::vector<std::string> owners;
	std::vector<std::string> management;
	std::vector<std::string> relatedCompanies;
	EarningsQuality earningsQuality;
	std::unordered_map<std::string, double> operationalMetrics;
};

class Company
{
public:
	explicit Company(std::string id);

	bool HasData(const std::string& method) const;
	void SetData(const std::string& method, nlohmann::json data);
	const nlohmann::json& GetData(const std::string& method) const;

	bool HasMetrics() const;
	void SetMetrics(CompanyMetrics metrics);
	const CompanyMetrics& GetMetrics() const;

	const std::string& GetIdentifier() const noexcept;
	std::size_t GetMethodCount() const noexcept;
	void PrintJson() const;
	void SaveToJson(const std::filesystem::path& path) const;

private:
	std::string m_id;
	std::unordered_map<std::string, nlohmann::json> m_data;

	bool m_hasMetrics = false;
	CompanyMetrics m_metrics;

	mutable std::mutex m_mutex;
};