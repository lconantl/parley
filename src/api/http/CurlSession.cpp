#include "CurlSession.hpp"

#include <curl/curl.h>
#include <stdexcept>
#include <utility>

namespace
{
constexpr long DefaultTimeoutSeconds = 30;
constexpr long ConnectTimeoutSeconds = 10;

void AssertIsHandleValid(const CURL* handle)
{
	if (handle == nullptr)
	{
		throw std::runtime_error("Не удалось создать HTTP-клиент");
	}
}

void AssertIsHeaderListValid(const curl_slist* headers)
{
	if (headers == nullptr)
	{
		throw std::runtime_error("Не удалось сформировать заголовки HTTP-запроса");
	}
}

void AssertIsUrlValid(const std::string& url)
{
	if (url.empty())
	{
		throw std::invalid_argument("Адрес запроса не может быть пустым");
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

size_t WriteResponse(
	const char* data,
	const size_t size,
	const size_t count,
	void* userData)
{
	auto* body = static_cast<std::string*>(userData);
	body->append(data, size * count);

	return size * count;
}

void ConfigureSecurity(CURL* handle)
{
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2L);
	curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
}

void ConfigureTimeouts(CURL* handle)
{
	curl_easy_setopt(handle, CURLOPT_TIMEOUT, DefaultTimeoutSeconds);
	curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, ConnectTimeoutSeconds);
}

long ReadStatusCode(CURL* handle)
{
	long statusCode = 0;
	curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &statusCode);

	return statusCode;
}
} // namespace

CurlSession::CurlSession()
	: m_handle(curl_easy_init())
	, m_headers(nullptr)
{
	AssertIsHandleValid(m_handle);
	ConfigureSecurity(m_handle);
	ConfigureTimeouts(m_handle);
}

CurlSession::~CurlSession()
{
	curl_slist_free_all(m_headers);
	curl_easy_cleanup(m_handle);
}

void CurlSession::SetUrl(const std::string& url) const
{
	AssertIsUrlValid(url);
	curl_easy_setopt(m_handle, CURLOPT_URL, url.c_str());
}

void CurlSession::SetTimeout(const long seconds) const
{
	curl_easy_setopt(m_handle, CURLOPT_TIMEOUT, seconds);
}

void CurlSession::AddHeader(const std::string& header)
{
	curl_slist* updated = curl_slist_append(m_headers, header.c_str());
	AssertIsHeaderListValid(updated);
	m_headers = updated;
}

void CurlSession::SetPostBody(std::string body)
{
	m_postBody = std::move(body);

	curl_easy_setopt(m_handle, CURLOPT_POST, 1L);
	curl_easy_setopt(m_handle, CURLOPT_POSTFIELDS, m_postBody.c_str());
	curl_easy_setopt(m_handle, CURLOPT_POSTFIELDSIZE, static_cast<long>(m_postBody.size()));
}

HttpResponse CurlSession::Perform() const
{
	std::string body;

	curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, WriteResponse);
	curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &body);
	curl_easy_setopt(m_handle, CURLOPT_HTTPHEADER, m_headers);

	AssertIsCurlResultValid(curl_easy_perform(m_handle));

	return {ReadStatusCode(m_handle), std::move(body)};
}