#include "CompanyAnalyticsViewModel.hpp"
#include "finance/FinancialCalculator.hpp"
#include "finance/FinancialHistoryParser.hpp"
#include "finance/Metric.hpp"
#include <algorithm>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
constexpr auto PartyMethod = "dadata-party";
constexpr auto FinancesMethod = "finances";
constexpr int NoYear = 0;
constexpr int ReportingLag = 1;
constexpr double LowConfidence = 0.6;

void AssertIsClientValid(const void* client, const std::string& message)
{
	if (client == nullptr)
	{
		throw std::invalid_argument(message);
	}
}

void AssertIsIdentifierValid(const std::string& identifier)
{
	if (identifier.empty())
	{
		throw std::invalid_argument("Идентификатор организации не может быть пустым");
	}
}

void ReportFailure(const std::string& stage, const std::exception& exception)
{
	std::cout << stage
			  << ": не удалось получить данные: "
			  << exception.what()
			  << std::endl;
}

int CurrentYear()
{
	const std::time_t now = std::time(nullptr);
	const std::tm* parts = std::gmtime(&now);

	if (parts == nullptr)
	{
		return NoYear;
	}

	return parts->tm_year + 1900;
}

void AddAssumption(CompanyAnalytics& analytics, const std::string& text)
{
	if (text.empty())
	{
		return;
	}

	const auto existing = std::find(
		analytics.assumptions.begin(),
		analytics.assumptions.end(),
		text);

	if (existing != analytics.assumptions.end())
	{
		return;
	}

	analytics.assumptions.push_back(text);
}
} // namespace

CompanyAnalyticsViewModel::CompanyAnalyticsViewModel(
	std::shared_ptr<DaDataApiClient> dadataClient,
	std::shared_ptr<CheckoApiClient> checkoClient,
	std::shared_ptr<PolzaClient> polzaClient)
	: m_dadataClient(std::move(dadataClient))
	, m_checkoClient(std::move(checkoClient))
{
	AssertIsClientValid(m_dadataClient.get(), "Клиент Дадаты не может быть пустым");
	AssertIsClientValid(m_checkoClient.get(), "Клиент Checko не может быть пустым");
	AssertIsClientValid(polzaClient.get(), "Клиент нейросети не может быть пустым");

	m_profileViewModel = std::make_shared<CompanyProfileViewModel>(m_dadataClient);
	m_estimator = std::make_shared<CompanyEstimator>(std::move(polzaClient));
}

CompanyAnalytics CompanyAnalyticsViewModel::Analyze(const std::string& identifier) const
{
	return Analyze(identifier, NoYear);
}

CompanyAnalytics CompanyAnalyticsViewModel::Analyze(
	const std::string& identifier,
	const int year) const
{
	AssertIsIdentifierValid(identifier);

	Company company(identifier);

	return Analyze(company, year);
}

CompanyAnalytics CompanyAnalyticsViewModel::Analyze(Company& company, const int year) const
{
	LoadProfile(company);
	LoadFinances(company);

	const FinancialHistory history = BuildHistory(company);
	const CompanyProfile profile = ReadProfile(company);
	const int selectedYear = SelectYear(history, year);

	CompanyAnalytics analytics = FinancialCalculator::Extract(history, profile, selectedYear);

	if (analytics.identifier.empty())
	{
		analytics.identifier = company.GetIdentifier();
	}

	FinancialCalculator::Derive(analytics);

	MetricIndex index(analytics);
	FillGaps(analytics, index);
	FinancialCalculator::Derive(analytics);

	analytics.status = index.EvaluateStatus();
	CollectAssumptions(analytics, index);

	company.SetAnalytics(analytics);

	return analytics;
}

void CompanyAnalyticsViewModel::LoadProfile(Company& company) const
{
	try
	{
		m_profileViewModel->Load(company);
	}
	catch (const std::exception& exception)
	{
		ReportFailure(PartyMethod, exception);
	}
}

void CompanyAnalyticsViewModel::LoadFinances(Company& company) const
{
	try
	{
		const CheckoResponse response = m_checkoClient->GetFinances(
			company.GetIdentifier(),
			{{"extended", "false"}});

		if (response.statusCode < 200 || response.statusCode >= 300)
		{
			throw std::runtime_error("Checko вернул ошибочный HTTP-код");
		}

		company.SetData(FinancesMethod, response.body);
	}
	catch (const std::exception& exception)
	{
		ReportFailure(FinancesMethod, exception);
	}
}

FinancialHistory CompanyAnalyticsViewModel::BuildHistory(const Company& company) const
{
	FinancialHistory checkoHistory;
	FinancialHistory dadataHistory;

	if (company.HasData(FinancesMethod))
	{
		checkoHistory = FinancialHistoryParser::ParseChecko(company.GetData(FinancesMethod));
	}

	if (company.HasData(PartyMethod))
	{
		dadataHistory = FinancialHistoryParser::ParseDaData(company.GetData(PartyMethod));
	}

	return FinancialHistoryParser::Merge(checkoHistory, dadataHistory);
}

CompanyProfile CompanyAnalyticsViewModel::ReadProfile(const Company& company) const
{
	if (!company.HasProfile())
	{
		CompanyProfile profile;
		profile.registry.inn = company.GetIdentifier();

		return profile;
	}

	return company.GetProfile();
}

int CompanyAnalyticsViewModel::SelectYear(const FinancialHistory& history, const int requestedYear)
{
	if (requestedYear != NoYear)
	{
		return requestedYear;
	}

	if (!history.IsEmpty())
	{
		return history.GetLatestYear();
	}

	return CurrentYear() - ReportingLag;
}

void CompanyAnalyticsViewModel::FillGaps(CompanyAnalytics& analytics, MetricIndex& index) const
{
	if (index.ListMissing().empty())
	{
		return;
	}

	try
	{
		const EstimationRequest request = CompanyEstimator::BuildRequest(analytics, index);
		const EstimationResult result = m_estimator->Estimate(request);

		CompanyEstimator::Apply(result, index, analytics);
	}
	catch (const std::exception& exception)
	{
		ReportFailure("estimation", exception);
	}
}

void CompanyAnalyticsViewModel::CollectAssumptions(
	CompanyAnalytics& analytics,
	const MetricIndex& index)
{
	for (const auto& descriptor : index.ListAll())
	{
		const MetricValue& metric = *descriptor.value;

		if (!Metric::IsKnown(metric) || metric.confidence >= LowConfidence)
		{
			continue;
		}

		if (metric.comment.empty())
		{
			continue;
		}

		AddAssumption(analytics, descriptor.title + ": " + metric.comment);
	}
}