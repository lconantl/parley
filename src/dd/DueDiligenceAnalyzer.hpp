#pragma once

#include "DueDiligenceTypes.hpp"
#include "bfo/BfoOrganization.hpp"

#include <nlohmann/json_fwd.hpp>

#include <string>
#include <unordered_map>
#include <vector>

class PolzaAiClient;

class DueDiligenceAnalyzer
{
public:
	DueDiligenceAnalyzer(
		BfoOrganizationProfile profile,
		std::vector<BfoPeriodReport> history,
		std::unordered_map<std::string, BfoDetailBreakdown> latestPeriodDetails,
		const PolzaAiClient& aiClient);

	[[nodiscard]]
	std::string BuildMarkdownReport() const;

private:
	[[nodiscard]]
	const BfoPeriodReport* FindLatestPeriodWithData() const;

	[[nodiscard]]
	const BfoCorrection* FindPrimaryCorrection(
		const BfoPeriodReport& period) const;

	[[nodiscard]]
	std::vector<DueDiligenceMetric> ComputeRevenueGrowthSeries() const;

	[[nodiscard]]
	DueDiligenceMetric ComputeCagr() const;

	[[nodiscard]]
	std::vector<DueDiligenceMetric> ComputeLatestPeriodMetrics(
		const BfoCorrection& correction) const;

	[[nodiscard]]
	std::vector<DueDiligenceFlag> DetectFlags(
		const BfoPeriodReport& latestPeriod,
		const BfoCorrection& correction) const;

	[[nodiscard]]
	nlohmann::json BuildAiContext(
		const std::vector<DueDiligenceMetric>& metrics,
		const std::vector<DueDiligenceFlag>& flags) const;

	[[nodiscard]]
	nlohmann::json RequestAiEnrichment(
		const nlohmann::json& context) const;

	[[nodiscard]]
	std::string RenderMarkdown(
		const std::vector<DueDiligenceMetric>& revenueSeries,
		const DueDiligenceMetric& cagr,
		const std::vector<DueDiligenceMetric>& latestMetrics,
		const std::vector<DueDiligenceFlag>& flags,
		const nlohmann::json& aiResult) const;

	BfoOrganizationProfile m_profile;
	std::vector<BfoPeriodReport> m_history;
	std::unordered_map<std::string, BfoDetailBreakdown> m_latestPeriodDetails;
	const PolzaAiClient& m_aiClient;
};
