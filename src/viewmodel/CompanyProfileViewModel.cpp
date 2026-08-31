#include "CompanyProfileViewModel.hpp"
#include "model/CompanyProfileParser.hpp"
#include <future>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace
{
constexpr auto PartyMethod = "dadata-party";
constexpr auto BrandMethod = "dadata-brand";
constexpr auto AffiliatedMethod = "dadata-affiliated";

constexpr std::size_t MaxAffiliationKeys = 5;
constexpr std::size_t MaxAffiliatedPerKey = 20;

void AssertIsApiClientValid(const std::shared_ptr<DaDataApiClient>& apiClient)
{
	if (apiClient == nullptr)
	{
		throw std::invalid_argument("API-клиент не может быть пустым");
	}
}

bool IsAccessDenied(const DaDataResponse& response)
{
	return response.statusCode == 401 || response.statusCode == 403;
}

void AssertIsResponseSuccessful(const DaDataResponse& response)
{
	if (response.statusCode == 401)
	{
		throw std::runtime_error("Дадата отклонила ключ доступа");
	}

	if (response.statusCode == 403)
	{
		throw std::runtime_error("Метод недоступен на текущем тарифе Дадаты");
	}

	if (response.statusCode == 429)
	{
		throw std::runtime_error("Превышен лимит запросов к Дадате");
	}

	if (response.statusCode < 200 || response.statusCode >= 300)
	{
		throw std::runtime_error(
			"Дадата вернула ошибочный HTTP-код: " + std::to_string(response.statusCode));
	}
}

void ReportFailure(const std::string& method, const std::exception& exception)
{
	std::cout << method
			  << ": не удалось получить данные: "
			  << exception.what()
			  << std::endl;
}

void AddAffiliationKey(
	std::vector<std::string>& keys,
	std::unordered_set<std::string>& visited,
	const std::string& identifier)
{
	if (identifier.empty() || keys.size() >= MaxAffiliationKeys)
	{
		return;
	}

	if (!visited.insert(identifier).second)
	{
		return;
	}

	keys.push_back(identifier);
}

std::vector<std::string> CollectAffiliationKeys(const CompanyProfile& profile)
{
	std::vector<std::string> keys;
	std::unordered_set<std::string> visited;

	AddAffiliationKey(keys, visited, profile.registry.inn);

	for (const auto& founder : profile.founders)
	{
		AddAffiliationKey(keys, visited, founder.inn);
	}

	for (const auto& manager : profile.employees.managers)
	{
		AddAffiliationKey(keys, visited, manager.inn);
	}

	return keys;
}

std::vector<AffiliatedCompany> MergeAffiliated(
	const std::vector<std::vector<AffiliatedCompany>>& batches,
	const std::string& ownIdentifier)
{
	std::vector<AffiliatedCompany> companies;
	std::unordered_set<std::string> visited;
	visited.insert(ownIdentifier);

	for (const auto& batch : batches)
	{
		for (const auto& company : batch)
		{
			if (company.inn.empty() || !visited.insert(company.inn).second)
			{
				continue;
			}

			companies.push_back(company);
		}
	}

	return companies;
}

void ApplyBrandProfile(CompanyProfile& profile, const BrandProfile& brandProfile)
{
	profile.brand = brandProfile.brand;
	profile.socials = brandProfile.socials;

	if (!brandProfile.website.empty())
	{
		profile.website = brandProfile.website;
	}
}
} // namespace

CompanyProfileViewModel::CompanyProfileViewModel(std::shared_ptr<DaDataApiClient> apiClient)
	: m_apiClient(std::move(apiClient))
{
	AssertIsApiClientValid(m_apiClient);
}

CompanyProfile CompanyProfileViewModel::LoadProfile(const std::string& identifier) const
{
	CompanyProfile profile = LoadParty(identifier);

	ApplyBrandProfile(profile, TryLoadBrand(identifier));
	profile.affiliatedCompanies = TryLoadAffiliatedCompanies(profile);

	return profile;
}

void CompanyProfileViewModel::Load(Company& company) const
{
	const std::string& identifier = company.GetIdentifier();

	const DaDataResponse partyResponse = m_apiClient->FindParty(identifier);
	AssertIsResponseSuccessful(partyResponse);
	company.SetData(PartyMethod, partyResponse.body);

	CompanyProfile profile = CompanyProfileParser::ParseParty(partyResponse.body);

	ApplyBrandProfile(profile, TryLoadBrand(identifier));
	profile.affiliatedCompanies = TryLoadAffiliatedCompanies(profile);

	company.SetProfile(std::move(profile));
}

CompanyProfile CompanyProfileViewModel::LoadParty(const std::string& identifier) const
{
	const DaDataResponse response = m_apiClient->FindParty(identifier);
	AssertIsResponseSuccessful(response);

	return CompanyProfileParser::ParseParty(response.body);
}

BrandProfile CompanyProfileViewModel::LoadBrand(const std::string& identifier) const
{
	const DaDataResponse response = m_apiClient->FindBrand(identifier);
	AssertIsResponseSuccessful(response);

	return CompanyProfileParser::ParseBrand(response.body);
}

std::vector<AffiliatedCompany> CompanyProfileViewModel::LoadAffiliated(
	const std::string& identifier) const
{
	const DaDataResponse response = m_apiClient->FindAffiliated(identifier, MaxAffiliatedPerKey);
	AssertIsResponseSuccessful(response);

	return CompanyProfileParser::ParseAffiliated(response.body, identifier);
}

BrandProfile CompanyProfileViewModel::TryLoadBrand(const std::string& identifier) const
{
	try
	{
		return LoadBrand(identifier);
	}
	catch (const std::exception& exception)
	{
		ReportFailure(BrandMethod, exception);
	}

	return {};
}

bool CompanyProfileViewModel::ProbeAffiliatedAccess(const std::string& identifier) const
{
	if (!m_affiliatedAvailable.load())
	{
		return false;
	}

	const DaDataResponse response = m_apiClient->FindAffiliated(identifier, MaxAffiliatedPerKey);
	if (!IsAccessDenied(response))
	{
		return true;
	}

	m_affiliatedAvailable.store(false);
	std::cout << AffiliatedMethod
			  << ": поиск аффилированных компаний доступен только на тарифе «Максимальный», "
				 "раздел будет пустым"
			  << std::endl;

	return false;
}

std::vector<AffiliatedCompany> CompanyProfileViewModel::TryLoadAffiliatedCompanies(
	const CompanyProfile& profile) const
{
	if (!ProbeAffiliatedAccess(profile.registry.inn))
	{
		return {};
	}

	const std::vector<std::string> keys = CollectAffiliationKeys(profile);

	std::vector<std::future<std::vector<AffiliatedCompany>>> tasks;
	tasks.reserve(keys.size());

	for (const auto& key : keys)
	{
		tasks.emplace_back(std::async(
			std::launch::async,
			[this, key] {
				try
				{
					return LoadAffiliated(key);
				}
				catch (const std::exception& exception)
				{
					ReportFailure(AffiliatedMethod, exception);
				}

				return std::vector<AffiliatedCompany>();
			}));
	}

	std::vector<std::vector<AffiliatedCompany>> batches;
	batches.reserve(tasks.size());

	for (auto& task : tasks)
	{
		batches.push_back(task.get());
	}

	return MergeAffiliated(batches, profile.registry.inn);
}