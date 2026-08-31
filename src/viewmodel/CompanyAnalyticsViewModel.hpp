#pragma once

#include "../finance/CompanyAnalytics.hpp"
#include "ai/CompanyEstimator.hpp"
#include "ai/PolzaClient.hpp"
#include "api/CheckoApiClient.hpp"
#include "api/DaDataApiClient.hpp"
#include "finance/FinancialHistory.hpp"
#include "finance/MetricIndex.hpp"
#include "model/Company.hpp"
#include "viewmodel/CompanyProfileViewModel.hpp"

#include <memory>
#include <string>

class CompanyAnalyticsViewModel
{
public:
	CompanyAnalyticsViewModel(
		std::shared_ptr<DaDataApiClient> dadataClient,
		std::shared_ptr<CheckoApiClient> checkoClient,
		std::shared_ptr<PolzaClient> polzaClient);

	CompanyAnalytics Analyze(const std::string& identifier) const;
	CompanyAnalytics Analyze(const std::string& identifier, int year) const;
	CompanyAnalytics Analyze(Company& company, int year) const;

private:
	void LoadProfile(Company& company) const;
	void LoadFinances(Company& company) const;
	FinancialHistory BuildHistory(const Company& company) const;
	CompanyProfile ReadProfile(const Company& company) const;

	static int SelectYear(const FinancialHistory& history, int requestedYear);
	static void CollectAssumptions(CompanyAnalytics& analytics, const MetricIndex& index);

	void FillGaps(CompanyAnalytics& analytics, MetricIndex& index) const;

	std::shared_ptr<DaDataApiClient> m_dadataClient;
	std::shared_ptr<CheckoApiClient> m_checkoClient;
	std::shared_ptr<CompanyProfileViewModel> m_profileViewModel;
	std::shared_ptr<CompanyEstimator> m_estimator;
};