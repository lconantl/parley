#include "CompanyViewModel.hpp"

#include <future>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
void AssertIsApiClientValid(
	const std::shared_ptr<CheckoApiClient>& apiClient)
{
	if (apiClient == nullptr)
	{
		throw std::invalid_argument("API-клиент не может быть пустым");
	}
}

void AssertIsResponseSuccessful(const CheckoResponse& response)
{
	if (response.statusCode < 200 || response.statusCode >= 300)
	{
		throw std::runtime_error(
			"Не удалось получить данные из API");
	}

	if (!response.body.contains("meta"))
	{
		throw std::runtime_error(
			"Ответ API не содержит метаданные");
	}

	const auto& meta = response.body.at("meta");

	if (meta.contains("status") && meta.at("status") != "ok")
	{
		if (meta.contains("message"))
		{
			throw std::runtime_error(
				meta.at("message").get<std::string>());
		}

		throw std::runtime_error(
			"API вернул ошибочный статус");
	}
}
} // namespace

CompanyViewModel::CompanyViewModel(
	std::shared_ptr<CheckoApiClient> apiClient)
	: m_apiClient(std::move(apiClient))
{
	AssertIsApiClientValid(m_apiClient);
}

std::shared_ptr<Company> CompanyViewModel::LoadCompany(
	const std::string& identifier) const
{
	auto company = std::make_shared<Company>(identifier);

	std::vector<std::future<void>> tasks;

	tasks.emplace_back(std::async(
		std::launch::async,
		[this, &company] {
			TryLoad(*company, "company");
		}));

	tasks.emplace_back(std::async(
		std::launch::async,
		[this, &company] {
			TryLoad(*company, "timeline");
		}));

	tasks.emplace_back(std::async(
		std::launch::async,
		[this, &company] {
			TryLoad(*company, "finances");
		}));

	tasks.emplace_back(std::async(
		std::launch::async,
		[this, &company] {
			TryLoad(*company, "contracts");
		}));

	tasks.emplace_back(std::async(
		std::launch::async,
		[this, &company] {
			TryLoad(*company, "inspections");
		}));

	tasks.emplace_back(std::async(
		std::launch::async,
		[this, &company] {
			TryLoad(*company, "enforcements");
		}));

	tasks.emplace_back(std::async(
		std::launch::async,
		[this, &company] {
			TryLoad(*company, "legal-cases");
		}));

	tasks.emplace_back(std::async(
		std::launch::async,
		[this, &company] {
			TryLoad(*company, "fedresurs");
		}));

	tasks.emplace_back(std::async(
		std::launch::async,
		[this, &company] {
			TryLoad(*company, "bankruptcy-messages");
		}));

	for (auto& task : tasks)
	{
		task.get();
	}

	return company;
}

void CompanyViewModel::TryLoad(
	Company& company,
	const std::string& method) const
{
	try
	{
		if (method == "company")
		{
			LoadCompanyData(company);
		}
		else if (method == "timeline")
		{
			LoadTimelineData(company);
		}
		else if (method == "finances")
		{
			LoadFinancesData(company);
		}
		else if (method == "contracts")
		{
			LoadContractsData(company);
		}
		else if (method == "inspections")
		{
			LoadInspectionsData(company);
		}
		else if (method == "enforcements")
		{
			LoadEnforcementsData(company);
		}
		else if (method == "legal-cases")
		{
			LoadLegalCasesData(company);
		}
		else if (method == "fedresurs")
		{
			LoadFedresursData(company);
		}
		else if (method == "bankruptcy-messages")
		{
			LoadBankruptcyMessagesData(company);
		}
	}
	catch (const std::exception& exception)
	{
		std::cout << method
				  << ": не удалось получить данные: "
				  << exception.what()
				  << std::endl;
	}
}

void CompanyViewModel::LoadCompanyData(Company& company) const
{
	const auto response = m_apiClient->GetCompany(
		company.GetIdentifier());

	AssertIsResponseSuccessful(response);

	company.SetData("company", response.body);
}

void CompanyViewModel::LoadTimelineData(Company& company) const
{
	const auto response = m_apiClient->GetTimeline(
		company.GetIdentifier(),
		{});

	AssertIsResponseSuccessful(response);

	company.SetData("timeline", response.body);
}

void CompanyViewModel::LoadFinancesData(Company& company) const
{
	const auto response = m_apiClient->GetFinances(
		company.GetIdentifier(),
		{});

	AssertIsResponseSuccessful(response);

	company.SetData("finances", response.body);
}

void CompanyViewModel::LoadContractsData(Company& company) const
{
	const auto response = m_apiClient->GetContracts(
		company.GetIdentifier(),
		{});

	AssertIsResponseSuccessful(response);

	company.SetData("contracts", response.body);
}

void CompanyViewModel::LoadInspectionsData(Company& company) const
{
	const auto response = m_apiClient->GetInspections(
		company.GetIdentifier(),
		{});

	AssertIsResponseSuccessful(response);

	company.SetData("inspections", response.body);
}

void CompanyViewModel::LoadEnforcementsData(Company& company) const
{
	const auto response = m_apiClient->GetEnforcements(
		company.GetIdentifier(),
		{});

	AssertIsResponseSuccessful(response);

	company.SetData("enforcements", response.body);
}

void CompanyViewModel::LoadLegalCasesData(Company& company) const
{
	const auto response = m_apiClient->GetLegalCases(
		company.GetIdentifier(),
		{});

	AssertIsResponseSuccessful(response);

	company.SetData("legal-cases", response.body);
}

void CompanyViewModel::LoadFedresursData(Company& company) const
{
	const auto response = m_apiClient->GetFedresurs(
		company.GetIdentifier(),
		{});

	AssertIsResponseSuccessful(response);

	company.SetData("fedresurs", response.body);
}

void CompanyViewModel::LoadBankruptcyMessagesData(Company& company) const
{
	const auto response = m_apiClient->GetBankruptcyMessages(
		company.GetIdentifier(),
		{});

	AssertIsResponseSuccessful(response);

	company.SetData("bankruptcy-messages", response.body);
}