#pragma once

#include "api/CheckoApiClient.hpp"
#include "model/Company.hpp"
#include <memory>

class CompanyViewModel
{
public:
	explicit CompanyViewModel(std::shared_ptr<CheckoApiClient> apiClient);
	std::shared_ptr<Company> LoadCompany(const std::string& identifier) const;

private:
	void LoadCompanyData(Company& company) const;
	void LoadTimelineData(Company& company) const;
	void LoadFinancesData(Company& company) const;
	void LoadContractsData(Company& company) const;
	void LoadInspectionsData(Company& company) const;
	void LoadEnforcementsData(Company& company) const;
	void LoadLegalCasesData(Company& company) const;
	void LoadFedresursData(Company& company) const;
	void LoadBankruptcyMessagesData(Company& company) const;

	void TryLoad(Company& company, const std::string& method) const;

	std::shared_ptr<CheckoApiClient> m_apiClient;
};