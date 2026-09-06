#include "CheckoApiClient.hpp"
#include <curl/curl.h>
#include <stdexcept>
#include <utility>

namespace
{
constexpr auto BaseUrl = "https://api.checko.ru/v2";

void AssertIsApiKeyValid(const std::string& apiKey)
{
	if (apiKey.empty())
	{
		throw std::invalid_argument("API-ключ не может быть пустым");
	}
}

void AssertIsIdentifierValid(const std::string& identifier)
{
	if (identifier.empty())
	{
		throw std::invalid_argument("Идентификатор организации не может быть пустым");
	}
}

void AssertIsCurlResultValid(const CURLcode result)
{
	if (result != CURLE_OK)
	{
		throw std::runtime_error(
			"Ошибка выполнения HTTP-запроса: " + std::string(curl_easy_strerror(result)));
	}
}

void AssertIsHttpStatusValid(const long statusCode)
{
	if (statusCode < 200 || statusCode >= 300)
	{
		throw std::runtime_error("Сервер вернул ошибочный HTTP-код");
	}
}

size_t WriteResponse(
	const char* data,
	const size_t size,
	const size_t count,
	void* userData)
{
	auto* response = static_cast<std::string*>(userData);
	response->append(data, size * count);
	return size * count;
}

std::string BuildUrl(
	const std::string& endpoint,
	const std::unordered_map<std::string, std::string>& parameters)
{
	std::string url = std::string(BaseUrl) + endpoint;
	bool firstParameter = true;

	for (const auto& [name, value] : parameters)
	{
		url += firstParameter ? "?" : "&";
		url += name;
		url += "=";
		url += value;

		firstParameter = false;
	}

	return url;
}

class CurlHandle
{
public:
	CurlHandle()
		: m_handle(curl_easy_init())
	{
		if (m_handle == nullptr)
		{
			throw std::runtime_error("Не удалось создать HTTP-клиент");
		}
	}

	~CurlHandle()
	{
		curl_easy_cleanup(m_handle);
	}

	CURL* Get() const noexcept
	{
		return m_handle;
	}

private:
	CURL* m_handle;
};

void ConfigureCommonRequest(
	CURL* handle,
	const std::string& url,
	std::string& response)
{
	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteResponse);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response);
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2L);
}

nlohmann::json ParseJson(const std::string& response)
{
	try
	{
		return nlohmann::json::parse(response);
	}
	catch (const nlohmann::json::parse_error&)
	{
		throw std::runtime_error("Сервер вернул некорректный JSON");
	}
}
} // namespace

class CheckoApiClient::Impl
{
public:
	explicit Impl(std::string apiKey)
		: m_apiKey(std::move(apiKey))
	{
		AssertIsApiKeyValid(m_apiKey);
		curl_global_init(CURL_GLOBAL_DEFAULT);
	}

	~Impl()
	{
		curl_global_cleanup();
	}

	CheckoResponse Request(const CheckoRequest& request)
	{
		CheckoRequest authenticatedRequest = request;
		authenticatedRequest.parameters["key"] = m_apiKey;

		CheckoResponse response = ExecuteRequest(authenticatedRequest);

		++m_statistics.requestCount;

		if (IsSuccessful(response))
		{
			++m_statistics.successfulRequestCount;
		}
		else
		{
			++m_statistics.failedRequestCount;
		}

		return response;
	}

	CheckoApiStatistics GetStatistics() const
	{
		return m_statistics;
	}

private:
	static CheckoResponse ExecuteRequest(const CheckoRequest& request)
	{
		const CurlHandle handle;
		std::string response;

		ConfigureRequest(handle.Get(), request, response);
		const CURLcode result = curl_easy_perform(handle.Get());
		AssertIsCurlResultValid(result);

		long statusCode = 0;
		curl_easy_getinfo(
			handle.Get(),
			CURLINFO_RESPONSE_CODE,
			&statusCode);

		return {
			statusCode,
			ParseJson(response)};
	}

	static void ConfigureRequest(
		CURL* handle,
		const CheckoRequest& request,
		std::string& response)
	{
		const std::string url = BuildUrl(
			request.endpoint,
			request.parameters);

		ConfigureCommonRequest(handle, url, response);

		if (request.method == CheckoHttpMethod::Post)
		{
			curl_easy_setopt(handle, CURLOPT_POST, 1L);
		}
	}

	static bool IsSuccessful(const CheckoResponse& response)
	{
		return response.statusCode >= 200 && response.statusCode < 300;
	}

private:
	std::string m_apiKey;
	CheckoApiStatistics m_statistics{};
};

CheckoApiClient::CheckoApiClient(std::string apiKey)
	: m_impl(std::make_unique<Impl>(std::move(apiKey)))
{
}

CheckoApiClient::~CheckoApiClient() = default;

CheckoResponse CheckoApiClient::GetCompany(
	const std::string& identifier) const
{
	AssertIsIdentifierValid(identifier);

	return m_impl->Request({"/company",
		CheckoHttpMethod::Get,
		{{"inn", identifier}}});
}

CheckoResponse CheckoApiClient::GetEntrepreneur(
	const std::string& identifier) const
{
	AssertIsIdentifierValid(identifier);

	return m_impl->Request({"/entrepreneur",
		CheckoHttpMethod::Get,
		{{"inn", identifier}}});
}

CheckoResponse CheckoApiClient::GetPerson(
	const std::string& identifier) const
{
	AssertIsIdentifierValid(identifier);

	return m_impl->Request({"/person",
		CheckoHttpMethod::Get,
		{{"inn", identifier}}});
}

CheckoResponse CheckoApiClient::GetTimeline(
	const std::string& identifier,
	const std::unordered_map<std::string, std::string>& parameters) const
{
	AssertIsIdentifierValid(identifier);

	auto requestParameters = parameters;
	requestParameters["inn"] = identifier;

	return m_impl->Request({"/timeline",
		CheckoHttpMethod::Get,
		requestParameters});
}

CheckoResponse CheckoApiClient::Search(
	const std::unordered_map<std::string, std::string>& parameters) const
{
	return m_impl->Request({"/search",
		CheckoHttpMethod::Get,
		parameters});
}

CheckoResponse CheckoApiClient::GetFinances(
	const std::string& identifier,
	const std::unordered_map<std::string, std::string>& parameters) const
{
	AssertIsIdentifierValid(identifier);

	auto requestParameters = parameters;
	requestParameters["inn"] = identifier;

	return m_impl->Request({"/finances",
		CheckoHttpMethod::Get,
		requestParameters});
}

CheckoResponse CheckoApiClient::GetContracts(
	const std::string& identifier,
	const std::unordered_map<std::string, std::string>& parameters) const
{
	AssertIsIdentifierValid(identifier);

	auto requestParameters = parameters;
	requestParameters["inn"] = identifier;

	return m_impl->Request({"/contracts",
		CheckoHttpMethod::Get,
		requestParameters});
}

CheckoResponse CheckoApiClient::GetInspections(
	const std::string& identifier,
	const std::unordered_map<std::string, std::string>& parameters) const
{
	AssertIsIdentifierValid(identifier);

	auto requestParameters = parameters;
	requestParameters["inn"] = identifier;

	return m_impl->Request({"/inspections",
		CheckoHttpMethod::Get,
		requestParameters});
}

CheckoResponse CheckoApiClient::GetEnforcements(
	const std::string& identifier,
	const std::unordered_map<std::string, std::string>& parameters) const
{
	AssertIsIdentifierValid(identifier);

	auto requestParameters = parameters;
	requestParameters["inn"] = identifier;

	return m_impl->Request({"/enforcements",
		CheckoHttpMethod::Get,
		requestParameters});
}

CheckoResponse CheckoApiClient::GetLegalCases(
	const std::string& identifier,
	const std::unordered_map<std::string, std::string>& parameters) const
{
	AssertIsIdentifierValid(identifier);

	auto requestParameters = parameters;
	requestParameters["inn"] = identifier;

	return m_impl->Request({"/legal-cases",
		CheckoHttpMethod::Get,
		requestParameters});
}

CheckoResponse CheckoApiClient::GetFedresurs(
	const std::string& identifier,
	const std::unordered_map<std::string, std::string>& parameters) const
{
	AssertIsIdentifierValid(identifier);

	auto requestParameters = parameters;
	requestParameters["inn"] = identifier;

	return m_impl->Request({"/fedresurs",
		CheckoHttpMethod::Get,
		requestParameters});
}

CheckoResponse CheckoApiClient::GetBankruptcyMessages(
	const std::string& identifier,
	const std::unordered_map<std::string, std::string>& parameters) const
{
	AssertIsIdentifierValid(identifier);

	auto requestParameters = parameters;
	requestParameters["inn"] = identifier;

	return m_impl->Request({"/bankruptcy-messages",
		CheckoHttpMethod::Get,
		requestParameters});
}

CheckoResponse CheckoApiClient::GetBank(
	const std::unordered_map<std::string, std::string>& parameters) const
{
	return m_impl->Request({"/bank",
		CheckoHttpMethod::Get,
		parameters});
}

CheckoApiStatistics CheckoApiClient::GetStatistics() const
{
	return m_impl->GetStatistics();
}