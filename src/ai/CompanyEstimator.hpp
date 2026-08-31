#pragma once

#include "../finance/CompanyAnalytics.hpp"
#include "../finance/MetricIndex.hpp"
#include "PolzaClient.hpp"

#include <memory>
#include <string>
#include <vector>

struct MetricFact
{
	std::string id;
	std::string title;
	std::string unit;
	double value = 0.0;
};

struct MetricRequest
{
	std::string id;
	std::string title;
	std::string unit;
	std::string hint;
};

struct EstimationRequest
{
	std::string identifier;
	std::string name;
	std::string activity;
	std::string region;
	std::string legalForm;
	std::string brandSummary;
	std::string website;
	int year = 0;
	std::vector<MetricFact> facts;
	std::vector<MetricRequest> requests;
};

struct MetricEstimate
{
	std::string id;
	double value = 0.0;
	double confidence = 0.0;
	std::string basis;
};

struct EstimationResult
{
	bool companyFound = false;
	std::vector<MetricEstimate> estimates;
	std::vector<BusinessSegment> segments;
	std::vector<Competitor> competitors;
	std::vector<OperationalIndicator> operations;
	std::vector<LegalRisk> legalRisks;
	std::string marketDefinition;
	std::string researchNotes;
	double costRub = 0.0;
};

class CompanyEstimator
{
public:
	explicit CompanyEstimator(std::shared_ptr<PolzaClient> client);

	EstimationResult Estimate(const EstimationRequest& request) const;

	static EstimationRequest BuildRequest(const CompanyAnalytics& analytics, const MetricIndex& index);
	static void Apply(const EstimationResult& result, MetricIndex& index, CompanyAnalytics& analytics);

private:
	std::string Research(const EstimationRequest& request, double& cost) const;
	EstimationResult ExtractStructured(
		const EstimationRequest& request,
		const std::string& notes,
		double& cost) const;

	std::shared_ptr<PolzaClient> m_client;
};