#pragma once

#include "api/DaDataApiClient.hpp"
#include "model/Company.hpp"
#include "model/CompanyProfile.hpp"
#include <memory>
#include <string>
#include <vector>

class CompanyProfileViewModel
{
public:
	explicit CompanyProfileViewModel(std::shared_ptr<DaDataApiClient> apiClient);

	CompanyProfile LoadProfile(const std::string& identifier) const;
	void Load(Company& company) const;

private:
	CompanyProfile LoadParty(const std::string& identifier) const;
	BrandProfile LoadBrand(const std::string& identifier) const;
	std::vector<AffiliatedCompany> LoadAffiliated(const std::string& identifier) const;

	BrandProfile TryLoadBrand(const std::string& identifier) const;
	std::vector<AffiliatedCompany> TryLoadAffiliatedCompanies(const CompanyProfile& profile) const;

	std::shared_ptr<DaDataApiClient> m_apiClient;
};