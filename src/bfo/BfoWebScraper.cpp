#include "BfoWebScraper.hpp"

#include "config/Config.hpp"

#include <nlohmann/json.hpp>

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{

std::optional<std::string> OptString(
	const nlohmann::json& json,
	const char* key)
{
	const auto iterator = json.find(key);

	if (
		iterator == json.end()
		|| iterator->is_null())
	{
		return std::nullopt;
	}

	return iterator->get<std::string>();
}

std::optional<double> OptDouble(
	const nlohmann::json& json,
	const char* key)
{
	const auto iterator = json.find(key);

	if (
		iterator == json.end()
		|| iterator->is_null())
	{
		return std::nullopt;
	}

	return iterator->get<double>();
}

bool OptBool(
	const nlohmann::json& json,
	const char* key,
	const bool defaultValue)
{
	const auto iterator = json.find(key);

	if (
		iterator == json.end()
		|| iterator->is_null())
	{
		return defaultValue;
	}

	return iterator->get<bool>();
}

int OptInt(
	const nlohmann::json& json,
	const char* key,
	const int defaultValue)
{
	const auto iterator = json.find(key);

	if (
		iterator == json.end()
		|| iterator->is_null())
	{
		return defaultValue;
	}

	return iterator->get<int>();
}

int RequiredInt(
	const nlohmann::json& json,
	const char* key)
{
	return OptInt(
		json,
		key,
		0);
}

std::int64_t RequiredInt64(
	const nlohmann::json& json,
	const char* key)
{
	const auto iterator = json.find(key);

	if (
		iterator == json.end()
		|| iterator->is_null())
	{
		return 0;
	}

	return iterator->get<std::int64_t>();
}

std::string RequiredString(
	const nlohmann::json& json,
	const char* key)
{
	return OptString(
		json,
		key)
		.value_or("");
}

std::vector<int> IntArray(
	const nlohmann::json& json,
	const char* key)
{
	const auto iterator = json.find(key);

	if (
		iterator == json.end()
		|| !iterator->is_array())
	{
		return {};
	}

	std::vector<int> result;

	result.reserve(
		iterator->size());

	for (const auto& element : *iterator)
	{
		if (!element.is_null())
		{
			result.push_back(
				element.get<int>());
		}
	}

	return result;
}

BfoInfo ParseBfoInfo(
	const nlohmann::json& json)
{
	BfoInfo result;

	const auto iterator = json.find("bfo");

	if (
		iterator == json.end()
		|| iterator->is_null())
	{
		return result;
	}

	const auto& bfo = *iterator;

	result.period = OptString(
		bfo,
		"period");

	result.actualBfoDate = OptString(
		bfo,
		"actualBfoDate");

	result.gainSum = OptDouble(
		bfo,
		"gainSum");

	result.knd = OptString(
		bfo,
		"knd");

	result.hasAz = OptBool(
		bfo,
		"hasAz",
		false);

	result.hasKs = OptBool(
		bfo,
		"hasKs",
		false);

	result.actualCorrectionNumber = OptInt(
		bfo,
		"actualCorrectionNumber",
		0);

	result.actualCorrectionDate = OptString(
		bfo,
		"actualCorrectionDate");

	result.isCb = OptBool(
		bfo,
		"isCb",
		false);

	result.bfoPeriodTypes = IntArray(
		bfo,
		"bfoPeriodTypes");

	return result;
}

} // namespace

BfoWebScraper::BfoWebScraper(
	const Config& config)
	: m_httpClient(
		  config.GetBfoBaseUrl(),
		  config.GetBfoConnectTimeoutSec(),
		  config.GetBfoReadTimeoutSec(),
		  config.GetBfoWriteTimeoutSec(),
		  config.GetBfoUserAgent())
	, m_baseUrl(
		  config.GetBfoBaseUrl())
	, m_searchPath(
		  config.GetBfoSearchPath())
	, m_pageSize(
		  static_cast<int>(
			  config.GetBfoPageSize()))
{
}

SearchResponse BfoWebScraper::Search(
	const std::string& query,
	const int page) const
{
	if (query.empty())
	{
		throw std::runtime_error(
			"Поисковый запрос пуст");
	}

	const std::string path = m_searchPath
		+ "?query="
		+ UrlEncode(query)
		+ "&page="
		+ std::to_string(page)
		+ "&size="
		+ std::to_string(m_pageSize);

	const HttpResponse response = m_httpClient.Get(
		path,
		{
			{"Accept", "application/json"},
			{"Referer", "https://bo.nalog.gov.ru/"},
		});

	std::cout
		<< "[BFO] HTTP: "
		<< response.status
		<< ", body: "
		<< response.body.size()
		<< " bytes"
		<< std::endl;

	if (
		response.status < 200
		|| response.status >= 300)
	{
		throw std::runtime_error(
			"БФО вернул HTTP "
			+ std::to_string(
				response.status));
	}

	nlohmann::json root;

	try
	{
		root = nlohmann::json::parse(
			response.body);
	}
	catch (const nlohmann::json::parse_error& exception)
	{
		throw std::runtime_error(
			std::string(
				"Не удалось разобрать ответ БФО: ")
			+ exception.what());
	}

	return ParseSearchResponse(
		root,
		m_baseUrl);
}

SearchResponse BfoWebScraper::SearchByInn(
	const std::string& inn) const
{
	return Search(
		inn);
}

SearchResponse BfoWebScraper::SearchByName(
	const std::string& name) const
{
	return Search(
		name);
}

SearchResponse BfoWebScraper::ParseSearchResponse(
	const nlohmann::json& root,
	const std::string& baseUrl)
{
	SearchResponse result;

	result.pageNumber = RequiredInt(
		root,
		"number");

	result.pageSize = RequiredInt(
		root,
		"size");

	result.totalPages = RequiredInt(
		root,
		"totalPages");

	result.totalElements = RequiredInt64(
		root,
		"totalElements");

	result.numberOfElements = RequiredInt(
		root,
		"numberOfElements");

	result.last = OptBool(
		root,
		"last",
		false);

	result.first = OptBool(
		root,
		"first",
		false);

	result.empty = OptBool(
		root,
		"empty",
		false);

	const auto pageableIterator = root.find(
		"pageable");

	if (
		pageableIterator != root.end()
		&& !pageableIterator->is_null())
	{
		result.paged = OptBool(
			*pageableIterator,
			"paged",
			false);

		result.unpaged = OptBool(
			*pageableIterator,
			"unpaged",
			false);

		result.offset = RequiredInt64(
			*pageableIterator,
			"offset");
	}

	const auto contentIterator = root.find(
		"content");

	if (
		contentIterator != root.end()
		&& contentIterator->is_array())
	{
		result.companies.reserve(
			contentIterator->size());

		for (const auto& item : *contentIterator)
		{
			result.companies.push_back(
				ParseCompany(
					item,
					baseUrl));
		}
	}

	return result;
}

CompanySearchResult BfoWebScraper::ParseCompany(
	const nlohmann::json& item,
	const std::string& baseUrl)
{
	CompanySearchResult company;

	company.id = RequiredInt(
		item,
		"id");

	company.inn = StripTags(
		RequiredString(
			item,
			"inn"));

	company.shortName = StripTags(
		RequiredString(
			item,
			"shortName"));

	company.ogrn = StripTags(
		RequiredString(
			item,
			"ogrn"));

	company.index = OptString(
		item,
		"index");

	company.region = OptString(
		item,
		"region");

	company.district = OptString(
		item,
		"district");

	company.city = OptString(
		item,
		"city");

	company.settlement = OptString(
		item,
		"settlement");

	company.street = OptString(
		item,
		"street");

	company.house = OptString(
		item,
		"house");

	company.building = OptString(
		item,
		"building");

	company.office = OptString(
		item,
		"office");

	company.okved2 = RequiredString(
		item,
		"okved2");

	company.okopf = RequiredInt(
		item,
		"okopf");

	company.okato = OptString(
		item,
		"okato");

	company.okpo = OptString(
		item,
		"okpo");

	company.okfs = OptString(
		item,
		"okfs");

	company.statusCode = OptString(
		item,
		"statusCode");

	company.statusDate = OptString(
		item,
		"statusDate");

	company.bfo = ParseBfoInfo(
		item);

	company.cardUrl = baseUrl
		+ "/organizations-card/"
		+ std::to_string(
			  company.id);

	return company;
}

std::string BfoWebScraper::StripTags(
	const std::string& value)
{
	std::string result;

	result.reserve(
		value.size());

	bool insideTag = false;

	for (const char character : value)
	{
		if (character == '<')
		{
			insideTag = true;
			continue;
		}

		if (character == '>')
		{
			insideTag = false;
			continue;
		}

		if (!insideTag)
		{
			result += character;
		}
	}

	return result;
}

std::string BfoWebScraper::UrlEncode(
	const std::string& value)
{
	std::ostringstream output;

	output
		<< std::uppercase
		<< std::hex
		<< std::setfill('0');

	for (
		const unsigned char character :
		value)
	{
		if (
			std::isalnum(
				character)
			|| character == '-'
			|| character == '_'
			|| character == '.'
			|| character == '~')
		{
			output
				<< static_cast<char>(
					   character);
		}
		else
		{
			output
				<< '%'
				<< std::setw(2)
				<< static_cast<int>(
					   character);
		}
	}

	return output.str();
}
