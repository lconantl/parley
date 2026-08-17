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

constexpr const char* NBO_API_PREFIX = "/nbo";

std::optional<std::int64_t> OptInt64Value(
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

	return iterator->get<std::int64_t>();
}

std::optional<int> OptIntValue(
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

	return iterator->get<int>();
}

std::optional<bool> OptBoolValue(
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

	return iterator->get<bool>();
}

nlohmann::json OptJsonBlock(
	const nlohmann::json& json,
	const char* key)
{
	const auto iterator = json.find(key);

	if (
		iterator == json.end()
		|| iterator->is_null())
	{
		return nlohmann::json(nullptr);
	}

	return *iterator;
}

template <typename T, typename ParseFn>
std::optional<T> ParseOptional(
	const nlohmann::json& json,
	const char* key,
	ParseFn parse)
{
	const auto iterator = json.find(key);

	if (
		iterator == json.end()
		|| iterator->is_null())
	{
		return std::nullopt;
	}

	return parse(*iterator);
}

BfoCodeName ParseCodeName(
	const nlohmann::json& json)
{
	BfoCodeName result;

	const auto idIterator = json.find("id");

	if (
		idIterator != json.end()
		&& !idIterator->is_null())
	{
		result.id = idIterator->is_string()
			? idIterator->get<std::string>()
			: idIterator->dump();
	}

	result.name = OptString(
		json,
		"name")
					  .value_or("");

	return result;
}

BfoLocation ParseLocation(
	const nlohmann::json& json)
{
	BfoLocation result;

	result.id = OptIntValue(json, "id");
	result.name = OptString(json, "name");
	result.code = OptIntValue(json, "code");
	result.latitude = OptDouble(json, "latitude");
	result.longitude = OptDouble(json, "longitude");
	result.type = OptString(json, "type");
	result.parentId = OptIntValue(json, "parentId");

	return result;
}

BfoFileMetadata ParseFileMetadata(
	const nlohmann::json& json)
{
	BfoFileMetadata result;

	result.id = OptIntValue(json, "id");
	result.contentType = OptString(json, "contentType");
	result.size = OptInt64Value(json, "size");
	result.originalName = OptString(json, "originalName");
	result.fileToken = OptString(json, "fileToken");

	return result;
}

BfoAuditReport ParseAuditReport(
	const nlohmann::json& json)
{
	BfoAuditReport result;

	result.id = OptIntValue(json, "id");
	result.inn = OptString(json, "inn");
	result.ogrn = OptString(json, "ogrn");
	result.name = OptString(json, "name");
	result.isOrganization = OptBoolValue(json, "isOrganization");
	result.fileMetadata = ParseOptional<BfoFileMetadata>(json, "fileMetadata", ParseFileMetadata);

	return result;
}

BfoClarification ParseClarification(
	const nlohmann::json& json)
{
	BfoClarification result;

	result.id = OptIntValue(json, "id");
	result.fileMetadata = ParseOptional<BfoFileMetadata>(json, "fileMetadata", ParseFileMetadata);

	return result;
}

BfoOrganizationInfoRef ParseOrganizationInfoRef(
	const nlohmann::json& json)
{
	BfoOrganizationInfoRef result;

	result.fullName = OptString(json, "fullName");
	result.inn = OptString(json, "inn");
	result.kpp = OptString(json, "kpp");
	result.address = OptString(json, "address");
	result.okved2 = ParseOptional<BfoCodeName>(json, "okved2", ParseCodeName);
	result.okopf = ParseOptional<BfoCodeName>(json, "okopf", ParseCodeName);
	result.okfs = ParseOptional<BfoCodeName>(json, "okfs", ParseCodeName);
	result.okpo = OptString(json, "okpo");

	return result;
}

BfoCorrection ParseCorrection(
	const nlohmann::json& json)
{
	BfoCorrection result;

	result.id = RequiredInt(json, "id");
	result.bfoOrganizationInfo = ParseOrganizationInfoRef(
		OptJsonBlock(json, "bfoOrganizationInfo"));

	result.balance = OptJsonBlock(json, "balance");
	result.financialResult = OptJsonBlock(json, "financialResult");
	result.capitalChange = OptJsonBlock(json, "capitalChange");
	result.fundsMovement = OptJsonBlock(json, "fundsMovement");

	result.correctionVersion = OptInt(json, "correctionVersion", 0);
	result.requiredAudit = OptIntValue(json, "requiredAudit");
	result.datePresent = OptString(json, "datePresent");
	result.prBn = OptIntValue(json, "prBn");
	result.knd = OptString(json, "knd");
	result.auditReport = ParseOptional<BfoAuditReport>(json, "auditReport", ParseAuditReport);
	result.clarification = ParseOptional<BfoClarification>(json, "clarification", ParseClarification);
	result.periodType = OptIntValue(json, "periodType");

	return result;
}

BfoTypeCorrection ParseTypeCorrection(
	const nlohmann::json& json)
{
	BfoTypeCorrection result;

	result.type = OptInt(json, "type", 0);
	result.correction = ParseCorrection(
		OptJsonBlock(json, "correction"));

	return result;
}

BfoPeriodSummary ParsePeriodSummary(
	const nlohmann::json& json)
{
	BfoPeriodSummary result;

	result.period = RequiredString(json, "period");
	result.publication = OptInt(json, "publication", 0);
	result.actualBfoDate = OptString(json, "actualBfoDate");
	result.gainSum = OptDouble(json, "gainSum");
	result.knd = OptString(json, "knd");
	result.hasAz = OptBool(json, "hasAz", false);

	const auto hasKsIterator = json.find("hasKs");

	if (
		hasKsIterator != json.end()
		&& !hasKsIterator->is_null())
	{
		result.hasKs = hasKsIterator->get<bool>();
	}

	result.actualCorrectionNumber = OptInt(json, "actualCorrectionNumber", 0);
	result.actualCorrectionDate = OptString(json, "actualCorrectionDate");
	result.publishedCorrectionNumber = OptInt(json, "publishedCorrectionNumber", 0);
	result.publishedCorrectionDate = OptString(json, "publishedCorrectionDate");
	result.actives = OptDouble(json, "actives");
	result.isCb = OptBool(json, "isCb", false);
	result.mspCategory = OptString(json, "mspCategory");
	result.published = OptBool(json, "published", false);

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

BfoOrganizationProfile BfoWebScraper::GetOrganizationProfile(
	const int organizationId) const
{
	return ParseOrganizationProfile(
		FetchNboJson(
			"/organizations/" + std::to_string(organizationId)));
}

std::vector<BfoPeriodReport> BfoWebScraper::GetOrganizationBfoHistory(
	const int organizationId) const
{
	const nlohmann::json root = FetchNboJson(
		"/organizations/" + std::to_string(organizationId) + "/bfo/");

	std::vector<BfoPeriodReport> result;

	if (root.is_array())
	{
		result.reserve(root.size());

		for (const auto& item : root)
		{
			result.push_back(
				ParsePeriodReport(item));
		}
	}

	return result;
}

nlohmann::json BfoWebScraper::FetchNboJson(
	const std::string& path) const
{
	const HttpResponse response = m_httpClient.Get(
		NBO_API_PREFIX + path,
		{
			{"Accept", "application/json"},
			{"X-Requested-With", "XMLHttpRequest"},
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

	try
	{
		return nlohmann::json::parse(
			response.body);
	}
	catch (const nlohmann::json::parse_error& exception)
	{
		throw std::runtime_error(
			std::string(
				"Не удалось разобрать ответ БФО: ")
			+ exception.what());
	}
}

BfoOrganizationProfile BfoWebScraper::ParseOrganizationProfile(
	const nlohmann::json& root)
{
	BfoOrganizationProfile result;

	result.id = RequiredInt(root, "id");
	result.inn = RequiredString(root, "inn");
	result.shortName = RequiredString(root, "shortName");
	result.ogrn = RequiredString(root, "ogrn");
	result.index = OptString(root, "index");
	result.region = OptString(root, "region");
	result.district = OptString(root, "district");
	result.city = OptString(root, "city");
	result.settlement = OptString(root, "settlement");
	result.street = OptString(root, "street");
	result.house = OptString(root, "house");
	result.building = OptString(root, "building");
	result.office = OptString(root, "office");
	result.okved2 = ParseOptional<BfoCodeName>(root, "okved2", ParseCodeName);
	result.okopf = ParseOptional<BfoCodeName>(root, "okopf", ParseCodeName);

	const auto bfoIterator = root.find("bfo");

	if (
		bfoIterator != root.end()
		&& bfoIterator->is_array())
	{
		result.bfo.reserve(bfoIterator->size());

		for (const auto& item : *bfoIterator)
		{
			result.bfo.push_back(
				ParsePeriodSummary(item));
		}
	}

	result.okato = OptString(root, "okato");
	result.okpo = OptString(root, "okpo");
	result.okfs = OptString(root, "okfs");
	result.statusCode = OptString(root, "statusCode");
	result.statusDate = OptString(root, "statusDate");
	result.msp = OptString(root, "msp");
	result.kpp = OptString(root, "kpp");
	result.fullName = OptString(root, "fullName");
	result.registrationDate = OptString(root, "registrationDate");
	result.location = ParseOptional<BfoLocation>(root, "location", ParseLocation);
	result.authorizedCapital = OptDouble(root, "authorizedCapital");
	result.active = OptBool(root, "active", false);

	return result;
}

BfoPeriodReport BfoWebScraper::ParsePeriodReport(
	const nlohmann::json& item)
{
	BfoPeriodReport result;

	result.id = RequiredInt(item, "id");
	result.period = RequiredString(item, "period");
	result.publication = OptInt(item, "publication", 0);
	result.actualBfoDate = OptString(item, "actualBfoDate");
	result.gainSum = OptDouble(item, "gainSum");
	result.knd = OptString(item, "knd");
	result.hasAz = OptBool(item, "hasAz", false);

	const auto hasKsIterator = item.find("hasKs");

	if (
		hasKsIterator != item.end()
		&& !hasKsIterator->is_null())
	{
		result.hasKs = hasKsIterator->get<bool>();
	}

	result.actualCorrectionNumber = OptInt(item, "actualCorrectionNumber", 0);
	result.actualCorrectionDate = OptString(item, "actualCorrectionDate");
	result.publishedCorrectionNumber = OptInt(item, "publishedCorrectionNumber", 0);
	result.publishedCorrectionDate = OptString(item, "publishedCorrectionDate");
	result.actives = OptDouble(item, "actives");
	result.isCb = OptBool(item, "isCb", false);
	result.mspCategory = OptString(item, "mspCategory");
	result.organizationInfo = ParseOrganizationInfoRef(
		OptJsonBlock(item, "organizationInfo"));

	const auto typeCorrectionsIterator = item.find("typeCorrections");

	if (
		typeCorrectionsIterator != item.end()
		&& typeCorrectionsIterator->is_array())
	{
		result.typeCorrections.reserve(typeCorrectionsIterator->size());

		for (const auto& correctionItem : *typeCorrectionsIterator)
		{
			result.typeCorrections.push_back(
				ParseTypeCorrection(correctionItem));
		}
	}

	result.published = OptBool(item, "published", false);

	return result;
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
