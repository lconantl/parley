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
		const SearchResponse& response
	);

private:
	[[nodiscard]]
	static std::string FormatCompany(
		const CompanySearchResult& company
	);

	[[nodiscard]]
	static std::string OrDash(
		const std::optional<std::string>& value
	);

	[[nodiscard]]
	static std::string FormatGainSum(
		const std::optional<double>& value
	);

	[[nodiscard]]
	static std::string EscapeHtml(
		const std::string& value
	);
};
