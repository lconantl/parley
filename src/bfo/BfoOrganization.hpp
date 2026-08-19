#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct BfoDetailItem
{
	int parentCode = 0;
	std::string codeName;
	int code = 0;
	std::optional<std::string> expl;
	std::optional<double> current;
	std::optional<double> previous;
};

using BfoDetailBreakdown = std::unordered_map<std::string, std::vector<BfoDetailItem>>;

struct BfoCodeName
{
	std::string id;
	std::string name;
};

struct BfoLocation
{
	std::optional<int> id;
	std::optional<std::string> name;
	std::optional<int> code;
	std::optional<double> latitude;
	std::optional<double> longitude;
	std::optional<std::string> type;
	std::optional<int> parentId;
};

struct BfoPeriodSummary
{
	std::string period;
	int publication = 0;
	std::optional<std::string> actualBfoDate;
	std::optional<double> gainSum;
	std::optional<std::string> knd;
	bool hasAz = false;
	std::optional<bool> hasKs;
	int actualCorrectionNumber = 0;
	std::optional<std::string> actualCorrectionDate;
	int publishedCorrectionNumber = 0;
	std::optional<std::string> publishedCorrectionDate;
	std::optional<double> actives;
	bool isCb = false;
	std::optional<std::string> mspCategory;
	bool published = false;
};

struct BfoOrganizationProfile
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
	std::optional<BfoCodeName> okved2;
	std::optional<BfoCodeName> okopf;
	std::vector<BfoPeriodSummary> bfo;
	std::optional<std::string> okato;
	std::optional<std::string> okpo;
	std::optional<std::string> okfs;
	std::optional<std::string> statusCode;
	std::optional<std::string> statusDate;
	std::optional<std::string> msp;
	std::optional<std::string> kpp;
	std::optional<std::string> fullName;
	std::optional<std::string> registrationDate;
	std::optional<BfoLocation> location;
	std::optional<double> authorizedCapital;
	bool active = false;

	// Всё, что API вернул в корне объекта, но чего нет среди полей выше —
	// чтобы ни один факт из ответа сервера не терялся молча.
	nlohmann::json extra;
};

struct BfoOrganizationInfoRef
{
	std::optional<std::string> fullName;
	std::optional<std::string> inn;
	std::optional<std::string> kpp;
	std::optional<std::string> address;
	std::optional<BfoCodeName> okved2;
	std::optional<BfoCodeName> okopf;
	std::optional<BfoCodeName> okfs;
	std::optional<std::string> okpo;
};

struct BfoFileMetadata
{
	std::optional<int> id;
	std::optional<std::string> contentType;
	std::optional<std::int64_t> size;
	std::optional<std::string> originalName;
	std::optional<std::string> fileToken;
};

struct BfoAuditReport
{
	std::optional<int> id;
	std::optional<std::string> inn;
	std::optional<std::string> ogrn;
	std::optional<std::string> name;
	std::optional<bool> isOrganization;
	std::optional<BfoFileMetadata> fileMetadata;
};

struct BfoClarification
{
	std::optional<int> id;
	std::optional<BfoFileMetadata> fileMetadata;
};

struct BfoCorrection
{
	int id = 0;
	BfoOrganizationInfoRef bfoOrganizationInfo;

	// Сырые строки формы БФО (ОКУД 0710001/0710002/0710004/0710005) —
	// передаются как есть, без ручного перечисления полей.
	nlohmann::json balance;
	nlohmann::json financialResult;
	nlohmann::json capitalChange;
	nlohmann::json fundsMovement;

	int correctionVersion = 0;
	std::optional<int> requiredAudit;
	std::optional<std::string> datePresent;
	std::optional<int> prBn;
	std::optional<std::string> knd;
	std::optional<BfoAuditReport> auditReport;
	std::optional<BfoClarification> clarification;
	std::optional<int> periodType;

	nlohmann::json extra;
};

struct BfoTypeCorrection
{
	int type = 0;
	BfoCorrection correction;
};

struct BfoPeriodReport
{
	int id = 0;
	std::string period;
	int publication = 0;
	std::optional<std::string> actualBfoDate;
	std::optional<double> gainSum;
	std::optional<std::string> knd;
	bool hasAz = false;
	std::optional<bool> hasKs;
	int actualCorrectionNumber = 0;
	std::optional<std::string> actualCorrectionDate;
	int publishedCorrectionNumber = 0;
	std::optional<std::string> publishedCorrectionDate;
	std::optional<double> actives;
	bool isCb = false;
	std::optional<std::string> mspCategory;
	BfoOrganizationInfoRef organizationInfo;
	std::vector<BfoTypeCorrection> typeCorrections;
	bool published = false;

	nlohmann::json extra;
};
