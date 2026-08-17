#pragma once

#include "BfoWebScraper.hpp"

#include <optional>
#include <string>
#include <vector>

class BfoFormatter
{
public:
	[[nodiscard]]
	static std::vector<std::string> Format(
		const SearchResponse& response);

	[[nodiscard]]
	static std::vector<std::string> FormatProfile(
		const BfoOrganizationProfile& profile,
		const std::vector<BfoPeriodReport>& history);

private:
	[[nodiscard]]
	static std::string FormatCompany(
		const CompanySearchResult& company);

	[[nodiscard]]
	static std::string FormatProfileCard(
		const BfoOrganizationProfile& profile,
		const std::vector<BfoPeriodReport>& history);

	[[nodiscard]]
	static std::string FormatCodeName(
		const std::optional<BfoCodeName>& value);

	[[nodiscard]]
	static std::string OrDash(
		const std::optional<std::string>& value);

	[[nodiscard]]
	static std::string FormatGainSum(
		const std::optional<double>& value);

	[[nodiscard]]
	static std::string FormatJsonAmount(
		const nlohmann::json& block,
		const char* key);

	[[nodiscard]]
	static std::string EscapeHtml(
		const std::string& value);
};
