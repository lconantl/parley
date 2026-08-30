#pragma once

#include "CompanyProfile.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class CompanyProfileParser
{
public:
	static CompanyProfile ParseParty(const nlohmann::json& response);
	static BrandProfile ParseBrand(const nlohmann::json& response);

	static std::vector<AffiliatedCompany> ParseAffiliated(
		const nlohmann::json& response,
		const std::string& relatedIdentifier);
};