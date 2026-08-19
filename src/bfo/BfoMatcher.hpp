#pragma once

#include "BfoWebScraper.hpp"

#include <string>
#include <vector>

class BfoMatcher
{
public:
	[[nodiscard]]
	static std::vector<CompanySearchResult> FilterByQuery(
		const std::vector<CompanySearchResult>& companies,
		const std::string& query);

private:
	[[nodiscard]]
	static std::string NormalizeCore(
		const std::string& text);

	[[nodiscard]]
	static bool IsInnQuery(
		const std::string& query);
};
