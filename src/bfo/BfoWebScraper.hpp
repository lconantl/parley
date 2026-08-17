#pragma once

#include "http/HttpClient.hpp"

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class Config;

struct BfoInfo
{
	std::optional<std::string> period;
	std::optional<std::string> actualBfoDate;
	std::optional<double> gainSum;
	std::optional<std::string> knd;
	bool hasAz = false;
	bool hasKs = false;
	int actualCorrectionNumber = 0;
	std::optional<std::string> actualCorrectionDate;
	bool isCb = false;
	std::vector<int> bfoPeriodTypes;
};

struct CompanySearchResult
{
	int id = 0;

	std::string inn;
	std::string shortName;
	std::string ogrn;

	std::optional<std::string> index;
	std::optional<std::string> region;
	std::optional<std::string> district;
	std::optional<std::string> city;
	std::optional<std::string> settlement;
	std::optional<std::string> street;
	std::optional<std::string> house;
	std::optional<std::string> building;
	std::optional<std::string> office;

	std::string okved2;
	int okopf = 0;

	std::optional<std::string> okato;
	std::optional<std::string> okpo;
	std::optional<std::string> okfs;

	std::optional<std::string> statusCode;
	std::optional<std::string> statusDate;

	BfoInfo bfo;

	std::string cardUrl;
};

struct SearchResponse
{
	std::vector<CompanySearchResult> companies;

	int pageNumber = 0;
	int pageSize = 0;
	int totalPages = 0;
	std::int64_t totalElements = 0;
	int numberOfElements = 0;

	bool last = false;
	bool first = false;
	bool empty = false;
	bool paged = false;
	bool unpaged = false;

	std::int64_t offset = 0;
};

class BfoWebScraper
{
public:
	explicit BfoWebScraper(
		const Config& config
	);

	[[nodiscard]]
	SearchResponse Search(
		const std::string& query,
		int page = 0
	) const;

	[[nodiscard]]
	SearchResponse SearchByInn(
		const std::string& inn
	) const;

	[[nodiscard]]
	SearchResponse SearchByName(
		const std::string& name
	) const;

private:
	[[nodiscard]]
	static SearchResponse ParseSearchResponse(
		const nlohmann::json& root,
		const std::string& baseUrl
	);

	[[nodiscard]]
	static CompanySearchResult ParseCompany(
		const nlohmann::json& item,
		const std::string& baseUrl
	);

	[[nodiscard]]
	static std::string StripTags(
		const std::string& value
	);

	[[nodiscard]]
	static std::string UrlEncode(
		const std::string& value
	);

	HttpClient m_httpClient;
	std::string m_baseUrl;
	std::string m_searchPath;
	int m_pageSize;
};
