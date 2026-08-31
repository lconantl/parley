#pragma once

#include "CompanyAnalytics.hpp"
#include "MetricValue.hpp"
#include <string>
#include <vector>

struct MetricDescriptor
{
	std::string id;
	std::string title;
	MetricUnit unit = MetricUnit::Money;
	MetricGroup group = MetricGroup::Revenue;
	MetricValue* value = nullptr;
};

struct MetricSummary
{
	std::size_t total = 0;
	std::size_t reported = 0;
	std::size_t computed = 0;
	std::size_t estimated = 0;
	std::size_t missing = 0;
	std::size_t notApplicable = 0;
};

class MetricIndex
{
public:
	explicit MetricIndex(CompanyAnalytics& analytics);

	MetricIndex(const MetricIndex&) = delete;
	MetricIndex& operator=(const MetricIndex&) = delete;

	const std::vector<MetricDescriptor>& ListAll() const noexcept;
	std::vector<MetricDescriptor> ListMissing() const;
	MetricSummary Summarize() const;
	AnalyticsStatus EvaluateStatus() const;

	bool Apply(const std::string& id, const MetricValue& value);

private:
	void Register(const char* id, const char* title, MetricUnit unit, MetricValue& value);
	void OpenGroup(MetricGroup group) noexcept;
	void RegisterRevenue(CompanyAnalytics& analytics);
	void RegisterProfit(CompanyAnalytics& analytics);
	void RegisterMargins(CompanyAnalytics& analytics);
	void RegisterCashFlow(CompanyAnalytics& analytics);
	void RegisterWorkingCapital(CompanyAnalytics& analytics);
	void RegisterDebt(CompanyAnalytics& analytics);
	void RegisterBalance(CompanyAnalytics& analytics);
	void RegisterLiquidity(CompanyAnalytics& analytics);
	void RegisterReturns(CompanyAnalytics& analytics);
	void RegisterInvestment(CompanyAnalytics& analytics);
	void RegisterShareholders(CompanyAnalytics& analytics);
	void RegisterValuation(CompanyAnalytics& analytics);
	void RegisterQuality(CompanyAnalytics& analytics);
	void RegisterMarket(CompanyAnalytics& analytics);

	std::vector<MetricDescriptor> m_descriptors;
	MetricGroup m_group = MetricGroup::Revenue;
};
