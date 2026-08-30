#include "CompanyProfileParser.hpp"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace
{
constexpr auto SuggestionsField = "suggestions";
constexpr auto DataField = "data";

const nlohmann::json& EmptyObject()
{
	static const nlohmann::json value = nlohmann::json::object();

	return value;
}

const nlohmann::json& EmptyArray()
{
	static const nlohmann::json value = nlohmann::json::array();

	return value;
}

void AssertIsResponseValid(const nlohmann::json& response)
{
	if (!response.is_object())
	{
		throw std::invalid_argument("Ответ Дадаты должен быть объектом");
	}
}

void AssertIsSuggestionsPresent(const nlohmann::json& response)
{
	if (!response.contains(SuggestionsField) || !response.at(SuggestionsField).is_array())
	{
		throw std::runtime_error("Ответ Дадаты не содержит список результатов");
	}
}

void AssertIsSuggestionsNotEmpty(const nlohmann::json& suggestions)
{
	if (suggestions.empty())
	{
		throw std::runtime_error("Дадата не нашла организацию по указанному идентификатору");
	}
}

bool IsFilled(const nlohmann::json& node, const char* key)
{
	return node.is_object() && node.contains(key) && !node.at(key).is_null();
}

std::string ReadString(const nlohmann::json& node, const char* key)
{
	if (!IsFilled(node, key) || !node.at(key).is_string())
	{
		return {};
	}

	return node.at(key).get<std::string>();
}

double ReadNumber(const nlohmann::json& node, const char* key)
{
	if (!IsFilled(node, key) || !node.at(key).is_number())
	{
		return 0.0;
	}

	return node.at(key).get<double>();
}

std::int64_t ReadInteger(const nlohmann::json& node, const char* key)
{
	if (!IsFilled(node, key) || !node.at(key).is_number())
	{
		return 0;
	}

	return node.at(key).get<std::int64_t>();
}

bool ReadFlag(const nlohmann::json& node, const char* key)
{
	return IsFilled(node, key) && node.at(key).is_boolean() && node.at(key).get<bool>();
}

const nlohmann::json& ReadObject(const nlohmann::json& node, const char* key)
{
	if (!IsFilled(node, key) || !node.at(key).is_object())
	{
		return EmptyObject();
	}

	return node.at(key);
}

const nlohmann::json& ReadArray(const nlohmann::json& node, const char* key)
{
	if (!IsFilled(node, key) || !node.at(key).is_array())
	{
		return EmptyArray();
	}

	return node.at(key);
}

std::string PadNumber(const int value, const std::size_t width)
{
	std::string text = std::to_string(value);
	while (text.size() < width)
	{
		text.insert(text.begin(), '0');
	}

	return text;
}

std::string FormatTimestamp(const std::int64_t milliseconds)
{
	const std::chrono::sys_days days = std::chrono::floor<std::chrono::days>(
		std::chrono::sys_time<std::chrono::milliseconds>(
			std::chrono::milliseconds(milliseconds)));

	const std::chrono::year_month_day date(days);

	return PadNumber(static_cast<int>(static_cast<unsigned>(date.day())), 2)
		+ "." + PadNumber(static_cast<int>(static_cast<unsigned>(date.month())), 2)
		+ "." + std::to_string(static_cast<int>(date.year()));
}

std::string ReadDate(const nlohmann::json& node, const char* key)
{
	if (!IsFilled(node, key) || !node.at(key).is_number())
	{
		return {};
	}

	return FormatTimestamp(node.at(key).get<std::int64_t>());
}

PartyType ParsePartyType(const std::string& value)
{
	if (value == "LEGAL")
	{
		return PartyType::Legal;
	}

	if (value == "INDIVIDUAL")
	{
		return PartyType::Individual;
	}

	return PartyType::Unknown;
}

PartyStatus ParsePartyStatus(const std::string& value)
{
	if (value == "ACTIVE")
	{
		return PartyStatus::Active;
	}

	if (value == "LIQUIDATING")
	{
		return PartyStatus::Liquidating;
	}

	if (value == "LIQUIDATED")
	{
		return PartyStatus::Liquidated;
	}

	if (value == "BANKRUPT")
	{
		return PartyStatus::Bankrupt;
	}

	if (value == "REORGANIZING")
	{
		return PartyStatus::Reorganizing;
	}

	return PartyStatus::Unknown;
}

FounderType ParseFounderType(const std::string& value)
{
	if (value == "LEGAL")
	{
		return FounderType::Legal;
	}

	if (value == "PHYSICAL")
	{
		return FounderType::Physical;
	}

	return FounderType::Unknown;
}

ShareType ParseShareType(const std::string& value)
{
	if (value == "PERCENT")
	{
		return ShareType::Percent;
	}

	if (value == "DECIMAL")
	{
		return ShareType::Decimal;
	}

	if (value == "FRACTION")
	{
		return ShareType::Fraction;
	}

	return ShareType::Unknown;
}

void AppendNamePart(std::string& target, const std::string& part)
{
	if (part.empty())
	{
		return;
	}

	if (!target.empty())
	{
		target += " ";
	}

	target += part;
}

std::string ComposeFullName(const nlohmann::json& fio)
{
	std::string composed;
	AppendNamePart(composed, ReadString(fio, "surname"));
	AppendNamePart(composed, ReadString(fio, "name"));
	AppendNamePart(composed, ReadString(fio, "patronymic"));

	return composed;
}

std::string ParsePersonName(const nlohmann::json& node)
{
	const std::string name = ReadString(node, "name");
	if (!name.empty())
	{
		return name;
	}

	const nlohmann::json& fio = ReadObject(node, "fio");
	const std::string source = ReadString(fio, "source");
	if (!source.empty())
	{
		return source;
	}

	return ComposeFullName(fio);
}

bool ParseInvalidity(const nlohmann::json& node)
{
	return IsFilled(node, "invalidity");
}

FounderShare ParseShare(const nlohmann::json& node)
{
	const nlohmann::json& share = ReadObject(node, "share");

	FounderShare result;
	result.type = ParseShareType(ReadString(share, "type"));
	result.value = ReadNumber(share, "value");
	result.numerator = ReadInteger(share, "numerator");
	result.denominator = ReadInteger(share, "denominator");

	return result;
}

Founder ParseFounder(const nlohmann::json& node)
{
	Founder founder;
	founder.name = ParsePersonName(node);
	founder.inn = ReadString(node, "inn");
	founder.ogrn = ReadString(node, "ogrn");
	founder.type = ParseFounderType(ReadString(node, "type"));
	founder.share = ParseShare(node);
	founder.startDate = ReadDate(node, "start_date");
	founder.invalid = ParseInvalidity(node);

	return founder;
}

std::vector<Founder> ParseFounders(const nlohmann::json& data)
{
	std::vector<Founder> founders;
	for (const auto& node : ReadArray(data, "founders"))
	{
		founders.push_back(ParseFounder(node));
	}

	return founders;
}

Manager ParseManager(const nlohmann::json& node)
{
	Manager manager;
	manager.name = ParsePersonName(node);
	manager.post = ReadString(node, "post");
	manager.inn = ReadString(node, "inn");
	manager.ogrn = ReadString(node, "ogrn");
	manager.startDate = ReadDate(node, "start_date");
	manager.invalid = ParseInvalidity(node);

	return manager;
}

CompanyEmployees ParseEmployees(const nlohmann::json& data)
{
	CompanyEmployees employees;
	employees.count = static_cast<std::size_t>(ReadInteger(data, "employee_count"));

	for (const auto& node : ReadArray(data, "managers"))
	{
		employees.managers.push_back(ParseManager(node));
	}

	return employees;
}

std::vector<std::string> ParseStringList(const nlohmann::json& node, const char* key)
{
	std::vector<std::string> values;
	for (const auto& item : ReadArray(node, key))
	{
		if (item.is_string())
		{
			values.push_back(item.get<std::string>());
		}
	}

	return values;
}

License ParseLicense(const nlohmann::json& node)
{
	License license;
	license.series = ReadString(node, "series");
	license.number = ReadString(node, "number");
	license.issueDate = ReadDate(node, "issue_date");
	license.issueAuthority = ReadString(node, "issue_authority");
	license.suspendDate = ReadDate(node, "suspend_date");
	license.validFrom = ReadDate(node, "valid_from");
	license.validTo = ReadDate(node, "valid_to");
	license.activities = ParseStringList(node, "activities");
	license.addresses = ParseStringList(node, "addresses");

	return license;
}

std::vector<License> ParseLicenses(const nlohmann::json& data)
{
	std::vector<License> licenses;
	for (const auto& node : ReadArray(data, "licenses"))
	{
		licenses.push_back(ParseLicense(node));
	}

	return licenses;
}

std::vector<ActivityCode> ParseActivities(const nlohmann::json& data)
{
	std::vector<ActivityCode> activities;
	for (const auto& node : ReadArray(data, "okveds"))
	{
		ActivityCode activity;
		activity.code = ReadString(node, "code");
		activity.name = ReadString(node, "name");
		activity.main = ReadFlag(node, "main");

		activities.push_back(std::move(activity));
	}

	return activities;
}

std::string ParseAddress(const nlohmann::json& data)
{
	const nlohmann::json& address = ReadObject(data, "address");

	const std::string full = ReadString(address, "unrestricted_value");
	if (!full.empty())
	{
		return full;
	}

	return ReadString(address, "value");
}

std::string ParseWebsite(const nlohmann::json& data)
{
	const nlohmann::json& sites = ReadArray(data, "sites");
	if (sites.empty())
	{
		return {};
	}

	return ReadString(sites.front(), "value");
}

RegistryRecord ParseRegistry(const nlohmann::json& data)
{
	const nlohmann::json& name = ReadObject(data, "name");
	const nlohmann::json& opf = ReadObject(data, "opf");
	const nlohmann::json& state = ReadObject(data, "state");
	const nlohmann::json& capital = ReadObject(data, "capital");
	const nlohmann::json& finance = ReadObject(data, "finance");
	const nlohmann::json& management = ReadObject(data, "management");
	const nlohmann::json& smallBusiness = ReadObject(ReadObject(data, "documents"), "smb");

	RegistryRecord registry;
	registry.inn = ReadString(data, "inn");
	registry.kpp = ReadString(data, "kpp");
	registry.ogrn = ReadString(data, "ogrn");
	registry.okpo = ReadString(data, "okpo");
	registry.okato = ReadString(data, "okato");
	registry.oktmo = ReadString(data, "oktmo");
	registry.okogu = ReadString(data, "okogu");
	registry.okfs = ReadString(data, "okfs");
	registry.fullName = ReadString(name, "full_with_opf");
	registry.shortName = ReadString(name, "short_with_opf");
	registry.opf = ReadString(opf, "short");
	registry.type = ParsePartyType(ReadString(data, "type"));
	registry.status = ParsePartyStatus(ReadString(state, "status"));
	registry.registrationDate = ReadDate(state, "registration_date");
	registry.liquidationDate = ReadDate(state, "liquidation_date");
	registry.actualityDate = ReadDate(state, "actuality_date");
	registry.address = ParseAddress(data);
	registry.managementName = ReadString(management, "name");
	registry.managementPost = ReadString(management, "post");
	registry.taxSystem = ReadString(finance, "tax_system");
	registry.smallBusinessCategory = ReadString(smallBusiness, "category");
	registry.capital = ReadNumber(capital, "value");
	registry.branchCount = static_cast<std::size_t>(ReadInteger(data, "branch_count"));
	registry.activities = ParseActivities(data);

	return registry;
}

bool HasInvalidItem(const nlohmann::json& data, const char* key)
{
	for (const auto& node : ReadArray(data, key))
	{
		if (ParseInvalidity(node))
		{
			return true;
		}
	}

	return false;
}

void AddStatusRisks(const nlohmann::json& state, RiskMarks& risks)
{
	const PartyStatus status = ParsePartyStatus(ReadString(state, "status"));

	risks.isLiquidating = status == PartyStatus::Liquidating;
	risks.isLiquidated = status == PartyStatus::Liquidated;
	risks.isBankrupt = status == PartyStatus::Bankrupt;
	risks.isReorganizing = status == PartyStatus::Reorganizing;

	if (risks.isLiquidating)
	{
		risks.descriptions.emplace_back("Организация находится в процессе ликвидации");
	}

	if (risks.isLiquidated)
	{
		risks.descriptions.emplace_back("Организация ликвидирована");
	}

	if (risks.isBankrupt)
	{
		risks.descriptions.emplace_back("В отношении организации введена процедура банкротства");
	}

	if (risks.isReorganizing)
	{
		risks.descriptions.emplace_back("Организация присоединяется к другому юридическому лицу");
	}
}

void AddInvalidityRisks(const nlohmann::json& data, RiskMarks& risks)
{
	risks.hasInvalidData = ReadFlag(data, "invalid");
	risks.hasInvalidAddress = ParseInvalidity(ReadObject(data, "address"));
	risks.hasInvalidFounders = HasInvalidItem(data, "founders");
	risks.hasInvalidManagers = HasInvalidItem(data, "managers");

	if (risks.hasInvalidAddress)
	{
		risks.descriptions.emplace_back("Адрес признан недостоверным");
	}

	if (risks.hasInvalidFounders)
	{
		risks.descriptions.emplace_back("Сведения об учредителях признаны недостоверными");
	}

	if (risks.hasInvalidManagers)
	{
		risks.descriptions.emplace_back("Сведения о руководителях признаны недостоверными");
	}
}

void AddFinanceRisks(const nlohmann::json& finance, RiskMarks& risks)
{
	risks.taxDebt = ReadNumber(finance, "debt");
	risks.taxPenalty = ReadNumber(finance, "penalty");
	risks.hasTaxDebt = risks.taxDebt > 0.0;
	risks.hasTaxPenalty = risks.taxPenalty > 0.0;

	if (risks.hasTaxDebt)
	{
		risks.descriptions.emplace_back("Есть недоимка по налогам");
	}

	if (risks.hasTaxPenalty)
	{
		risks.descriptions.emplace_back("Есть налоговые штрафы");
	}
}

RiskMarks ParseRisks(const nlohmann::json& data)
{
	RiskMarks risks;
	AddStatusRisks(ReadObject(data, "state"), risks);
	AddInvalidityRisks(data, risks);
	AddFinanceRisks(ReadObject(data, "finance"), risks);

	return risks;
}

const nlohmann::json& ExtractFirstSuggestion(const nlohmann::json& response)
{
	AssertIsResponseValid(response);
	AssertIsSuggestionsPresent(response);

	const nlohmann::json& suggestions = response.at(SuggestionsField);
	AssertIsSuggestionsNotEmpty(suggestions);

	return suggestions.front();
}

SocialLinks ParseSocialLinks(const nlohmann::json& data)
{
	SocialLinks links;
	links.telegram = ReadString(data, "telegram_url");
	links.vk = ReadString(data, "vk_url");
	links.youtube = ReadString(data, "youtube_url");
	links.wildberries = ReadString(data, "wildberries_url");
	links.yandexMaps = ReadString(data, "yandex_maps_url");

	return links;
}

AffiliatedCompany ParseAffiliatedCompany(
	const nlohmann::json& suggestion,
	const std::string& relatedIdentifier)
{
	const nlohmann::json& data = ReadObject(suggestion, DataField);
	const nlohmann::json& name = ReadObject(data, "name");

	AffiliatedCompany company;
	company.name = ReadString(name, "short_with_opf");
	company.inn = ReadString(data, "inn");
	company.ogrn = ReadString(data, "ogrn");
	company.status = ParsePartyStatus(ReadString(ReadObject(data, "state"), "status"));
	company.address = ParseAddress(data);
	company.relatedIdentifier = relatedIdentifier;

	if (company.name.empty())
	{
		company.name = ReadString(suggestion, "value");
	}

	return company;
}
} // namespace

CompanyProfile CompanyProfileParser::ParseParty(const nlohmann::json& response)
{
	const nlohmann::json& data = ReadObject(ExtractFirstSuggestion(response), DataField);

	CompanyProfile profile;
	profile.registry = ParseRegistry(data);
	profile.founders = ParseFounders(data);
	profile.employees = ParseEmployees(data);
	profile.licenses = ParseLicenses(data);
	profile.risks = ParseRisks(data);
	profile.website = ParseWebsite(data);

	return profile;
}

BrandProfile CompanyProfileParser::ParseBrand(const nlohmann::json& response)
{
	const nlohmann::json& data = ReadObject(ExtractFirstSuggestion(response), DataField);

	BrandProfile profile;
	profile.brand.name = ReadString(data, "name");
	profile.brand.summary = ReadString(data, "summary");
	profile.brand.logoUrl = ReadString(data, "logo_url");
	profile.website = ReadString(data, "domain");
	profile.socials = ParseSocialLinks(data);

	return profile;
}

std::vector<AffiliatedCompany> CompanyProfileParser::ParseAffiliated(
	const nlohmann::json& response,
	const std::string& relatedIdentifier)
{
	AssertIsResponseValid(response);
	AssertIsSuggestionsPresent(response);

	std::vector<AffiliatedCompany> companies;
	for (const auto& suggestion : response.at(SuggestionsField))
	{
		companies.push_back(ParseAffiliatedCompany(suggestion, relatedIdentifier));
	}

	return companies;
}