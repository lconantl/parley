#include "DaDataApiClient.hpp"

#include "CurlSession.hpp"
#include "api/CurlGlobalScope.hpp"

#include <stdexcept>
#include <utility>

namespace
{
constexpr auto SuggestionsBaseUrl = "https://suggestions.dadata.ru/suggestions/api/4_1/rs";
constexpr auto ApiBaseUrl = "https://api.dadata.ru";
constexpr auto ProfileBaseUrl = "https://dadata.ru/api/v2";

constexpr auto FindPartyEndpoint = "/findById/party";
constexpr auto FindAffiliatedEndpoint = "/findAffiliated/party";
constexpr auto FindBrandEndpoint = "/findById/brand";
constexpr auto BalanceEndpoint = "/profile/balance";

constexpr auto MainBranchType = "MAIN";
constexpr std::size_t MaxResultCount = 300;
constexpr std::size_t MaxQueryLength = 300;

void AssertIsApiKeyValid(const std::string& apiKey)
{
	if (apiKey.empty())
	{
		throw std::invalid_argument("API-ключ не может быть пустым");
	}
}

void AssertIsSecretKeyValid(const std::string& secretKey)
{
	if (secretKey.empty())
	{
		throw std::invalid_argument("Секретный ключ не может быть пустым");
	}
}

void AssertIsIdentifierValid(const std::string& identifier)
{
	if (identifier.empty())
	{
		throw std::invalid_argument("Идентификатор организации не может быть пустым");
	}

	if (identifier.size() > MaxQueryLength)
	{
		throw std::invalid_argument("Идентификатор организации слишком длинный");
	}
}

void AssertIsResultCountValid(const std::size_t count)
{
	if (count == 0 || count > MaxResultCount)
	{
		throw std::invalid_argument("Количество результатов должно быть от 1 до 300");
	}
}

std::string SelectBaseUrl(const DaDataService service)
{
	switch (service)
	{
	case DaDataService::Suggestions:
		return SuggestionsBaseUrl;
	case DaDataService::Api:
		return ApiBaseUrl;
	case DaDataService::Profile:
		return ProfileBaseUrl;
	}

	throw std::invalid_argument("Неизвестный сервис Дадаты");
}

nlohmann::json TryParseJson(const std::string& body)
{
	if (body.empty())
	{
		return nlohmann::json::object();
	}

	try
	{
		return nlohmann::json::parse(body);
	}
	catch (const nlohmann::json::parse_error&)
	{
		return nlohmann::json::object();
	}
}

bool IsSuccessful(const long statusCode)
{
	return statusCode >= 200 && statusCode < 300;
}
} // namespace

class DaDataApiClient::Impl
{
public:
	Impl(std::string apiKey, std::string secretKey)
		: m_apiKey(std::move(apiKey))
		, m_secretKey(std::move(secretKey))
	{
		AssertIsApiKeyValid(m_apiKey);
		AssertIsSecretKeyValid(m_secretKey);
	}

	DaDataResponse Request(const DaDataRequest& request)
	{
		const DaDataResponse response = ExecuteRequest(request);
		RegisterResult(response.statusCode);

		return response;
	}

	DaDataApiStatistics GetStatistics() const
	{
		return m_statistics;
	}

private:
	DaDataResponse ExecuteRequest(const DaDataRequest& request) const
	{
		CurlSession session;

		session.SetUrl(SelectBaseUrl(request.service) + request.endpoint);
		ConfigureHeaders(session);
		ConfigureBody(session, request);

		const auto [statusCode, body] = session.Perform();

		return {statusCode, TryParseJson(body)};
	}

	void ConfigureHeaders(CurlSession& session) const
	{
		session.AddHeader("Content-Type: application/json");
		session.AddHeader("Accept: application/json");
		session.AddHeader("Authorization: Token " + m_apiKey);
		session.AddHeader("X-Secret: " + m_secretKey);
	}

	static void ConfigureBody(CurlSession& session, const DaDataRequest& request)
	{
		if (request.method != DaDataHttpMethod::Post)
		{
			return;
		}

		session.SetPostBody(request.body.dump());
	}

	void RegisterResult(const long statusCode)
	{
		++m_statistics.requestCount;

		if (IsSuccessful(statusCode))
		{
			++m_statistics.successfulRequestCount;
		}
		else
		{
			++m_statistics.failedRequestCount;
		}
	}

private:
	std::string m_apiKey;
	std::string m_secretKey;
	CurlGlobalScope m_curlScope;
	DaDataApiStatistics m_statistics{};
};

DaDataApiClient::DaDataApiClient(std::string apiKey, std::string secretKey)
	: m_impl(std::make_unique<Impl>(std::move(apiKey), std::move(secretKey)))
{
}

DaDataApiClient::~DaDataApiClient() = default;

DaDataResponse DaDataApiClient::FindParty(const std::string& identifier) const
{
	AssertIsIdentifierValid(identifier);

	return m_impl->Request({DaDataService::Suggestions,
		FindPartyEndpoint,
		DaDataHttpMethod::Post,
		{{"query", identifier}, {"branch_type", MainBranchType}, {"count", 1}}});
}

DaDataResponse DaDataApiClient::FindBranches(
	const std::string& identifier,
	const std::size_t count) const
{
	AssertIsIdentifierValid(identifier);
	AssertIsResultCountValid(count);

	return m_impl->Request({DaDataService::Suggestions,
		FindPartyEndpoint,
		DaDataHttpMethod::Post,
		{{"query", identifier}, {"count", count}}});
}

DaDataResponse DaDataApiClient::FindAffiliated(
	const std::string& identifier,
	const std::size_t count) const
{
	AssertIsIdentifierValid(identifier);
	AssertIsResultCountValid(count);

	return m_impl->Request({DaDataService::Suggestions,
		FindAffiliatedEndpoint,
		DaDataHttpMethod::Post,
		{{"query", identifier},
			{"count", count},
			{"scope", nlohmann::json::array({"FOUNDERS", "MANAGERS"})}}});
}

DaDataResponse DaDataApiClient::FindBrand(const std::string& identifier) const
{
	AssertIsIdentifierValid(identifier);

	return m_impl->Request({DaDataService::Api,
		FindBrandEndpoint,
		DaDataHttpMethod::Post,
		{{"query", identifier}}});
}

DaDataResponse DaDataApiClient::GetBalance() const
{
	return m_impl->Request({DaDataService::Profile,
		BalanceEndpoint,
		DaDataHttpMethod::Get,
		nlohmann::json::object()});
}

DaDataApiStatistics DaDataApiClient::GetStatistics() const
{
	return m_impl->GetStatistics();
}