#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class PartyType
{
	Unknown,
	Legal,
	Individual
};

enum class PartyStatus
{
	Unknown,
	Active,
	Liquidating,
	Liquidated,
	Bankrupt,
	Reorganizing
};

enum class FounderType
{
	Unknown,
	Legal,
	Physical
};

enum class ShareType
{
	Unknown,
	Percent,
	Decimal,
	Fraction
};

struct ActivityCode
{
	std::string code;
	std::string name;
	bool main = false;
};

struct RegistryRecord
{
	std::string inn;
	std::string kpp;
	std::string ogrn;
	std::string okpo;
	std::string okato;
	std::string oktmo;
	std::string okogu;
	std::string okfs;
	std::string fullName;
	std::string shortName;
	std::string opf;
	PartyType type = PartyType::Unknown;
	PartyStatus status = PartyStatus::Unknown;
	std::string registrationDate;
	std::string liquidationDate;
	std::string actualityDate;
	std::string address;
	std::string managementName;
	std::string managementPost;
	std::string taxSystem;
	std::string smallBusinessCategory;
	double capital = 0.0;
	std::size_t branchCount = 0;
	std::vector<ActivityCode> activities;
};

struct FounderShare
{
	ShareType type = ShareType::Unknown;
	double value = 0.0;
	std::int64_t numerator = 0;
	std::int64_t denominator = 0;
};

struct Founder
{
	std::string name;
	std::string inn;
	std::string ogrn;
	FounderType type = FounderType::Unknown;
	FounderShare share;
	std::string startDate;
	bool invalid = false;
};

struct AffiliatedCompany
{
	std::string name;
	std::string inn;
	std::string ogrn;
	PartyStatus status = PartyStatus::Unknown;
	std::string address;
	std::string relatedIdentifier;
};

struct Manager
{
	std::string name;
	std::string post;
	std::string inn;
	std::string ogrn;
	std::string startDate;
	bool invalid = false;
};

struct CompanyEmployees
{
	std::size_t count = 0;
	std::vector<Manager> managers;
};

struct License
{
	std::string series;
	std::string number;
	std::string issueDate;
	std::string issueAuthority;
	std::string suspendDate;
	std::string validFrom;
	std::string validTo;
	std::vector<std::string> activities;
	std::vector<std::string> addresses;
};

struct RiskMarks
{
	bool hasInvalidData = false;
	bool hasInvalidAddress = false;
	bool hasInvalidFounders = false;
	bool hasInvalidManagers = false;
	bool isLiquidating = false;
	bool isLiquidated = false;
	bool isBankrupt = false;
	bool isReorganizing = false;
	bool hasTaxDebt = false;
	bool hasTaxPenalty = false;
	double taxDebt = 0.0;
	double taxPenalty = 0.0;
	std::vector<std::string> descriptions;
};

struct CompanyBrand
{
	std::string name;
	std::string summary;
	std::string logoUrl;
};

struct SocialLinks
{
	std::string telegram;
	std::string vk;
	std::string youtube;
	std::string wildberries;
	std::string yandexMaps;
};

struct BrandProfile
{
	CompanyBrand brand;
	std::string website;
	SocialLinks socials;
};

struct CompanyProfile
{
	RegistryRecord registry;
	std::vector<Founder> founders;
	std::vector<AffiliatedCompany> affiliatedCompanies;
	CompanyEmployees employees;
	std::vector<License> licenses;
	RiskMarks risks;
	CompanyBrand brand;
	std::string website;
	SocialLinks socials;
};