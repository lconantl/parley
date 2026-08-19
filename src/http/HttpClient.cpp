#include "HttpClient.hpp"

#include <httplib.h>

#include <stdexcept>
#include <string>

namespace
{

struct ParsedUrl
{
	bool https = false;
	std::string host;
	int port = 0;
	std::string basePath;
};

ParsedUrl ParseUrl(const std::string& url)
{
	constexpr std::string_view HTTPS_PREFIX = "https://";
	constexpr std::string_view HTTP_PREFIX = "http://";

	ParsedUrl result;

	std::size_t position = 0;

	if (url.starts_with(HTTPS_PREFIX))
	{
		result.https = true;
		position = HTTPS_PREFIX.size();
	}
	else if (url.starts_with(HTTP_PREFIX))
	{
		result.https = false;
		position = HTTP_PREFIX.size();
	}
	else
	{
		throw std::runtime_error(
			"URL должен начинаться с http:// или https://");
	}

	const auto slashPosition = url.find('/', position);

	const std::string authority = slashPosition == std::string::npos
		? url.substr(position)
		: url.substr(
			  position,
			  slashPosition - position);

	if (authority.empty())
	{
		throw std::runtime_error("Пустой HTTP host");
	}

	if (slashPosition != std::string::npos)
	{
		std::string basePath = url.substr(slashPosition);

		if (
			basePath.size() > 1
			&& basePath.back() == '/')
		{
			basePath.pop_back();
		}

		result.basePath = std::move(basePath);
	}

	const auto colonPosition = authority.find(':');

	if (colonPosition == std::string::npos)
	{
		result.host = authority;
		result.port = result.https ? 443 : 80;
	}
	else
	{
		result.host = authority.substr(0, colonPosition);
		result.port = std::stoi(
			authority.substr(colonPosition + 1));
	}

	return result;
}

std::string NormalizePath(const std::string& path)
{
	if (path.empty())
	{
		return "/";
	}

	if (path.front() == '/')
	{
		return path;
	}

	return "/" + path;
}

void AddHeaders(
	httplib::Headers& headers,
	const std::string& userAgent,
	const std::unordered_map<std::string, std::string>& extra)
{
	headers.emplace(
		"User-Agent",
		userAgent);

	headers.emplace(
		"Accept",
		"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");

	headers.emplace(
		"Accept-Language",
		"ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7");

	for (const auto& [key, value] : extra)
	{
		headers.emplace(
			key,
			value);
	}
}

HttpResponse ConvertResponse(
	const httplib::Result& response)
{
	if (!response)
	{
		throw std::runtime_error(
			"HTTP request failed: "
			+ httplib::to_string(
				response.error()));
	}

	HttpResponse result;

	result.status = response->status;
	result.body = response->body;

	for (const auto& header : response->headers)
	{
		result.headers.emplace(
			header.first,
			header.second);
	}

	return result;
}

} // namespace

HttpClient::HttpClient(
	std::string baseUrl,
	const long connectTimeoutSec,
	const long readTimeoutSec,
	const long writeTimeoutSec,
	std::string userAgent)
	: m_baseUrl(std::move(baseUrl))
	, m_connectTimeoutSec(connectTimeoutSec)
	, m_readTimeoutSec(readTimeoutSec)
	, m_writeTimeoutSec(writeTimeoutSec)
	, m_userAgent(std::move(userAgent))
{
}

HttpResponse HttpClient::Get(
	const std::string& path,
	const std::unordered_map<std::string, std::string>& headers) const
{
	const ParsedUrl url = ParseUrl(m_baseUrl);

	const std::string requestPath = NormalizePath(url.basePath + path);

	httplib::Headers requestHeaders;

	AddHeaders(
		requestHeaders,
		m_userAgent,
		headers);

	if (url.https)
	{
		httplib::SSLClient client(
			url.host,
			url.port);

		client.enable_server_certificate_verification(
			true);

		client.set_connection_timeout(
			m_connectTimeoutSec,
			0);

		client.set_read_timeout(
			m_readTimeoutSec,
			0);

		client.set_write_timeout(
			m_writeTimeoutSec,
			0);

		client.set_follow_location(
			true);

		return ConvertResponse(
			client.Get(
				requestPath.c_str(),
				requestHeaders));
	}

	httplib::Client client(
		url.host,
		url.port);

	client.set_connection_timeout(
		m_connectTimeoutSec,
		0);

	client.set_read_timeout(
		m_readTimeoutSec,
		0);

	client.set_write_timeout(
		m_writeTimeoutSec,
		0);

	client.set_follow_location(
		true);

	return ConvertResponse(
		client.Get(
			requestPath.c_str(),
			requestHeaders));
}

HttpResponse HttpClient::Post(
	const std::string& path,
	const std::string& body,
	const std::string& contentType,
	const std::unordered_map<std::string, std::string>& headers) const
{
	const ParsedUrl url = ParseUrl(m_baseUrl);

	const std::string requestPath = NormalizePath(url.basePath + path);

	httplib::Headers requestHeaders;

	AddHeaders(
		requestHeaders,
		m_userAgent,
		headers);

	if (url.https)
	{
		httplib::SSLClient client(
			url.host,
			url.port);

		client.enable_server_certificate_verification(
			true);

		client.set_connection_timeout(
			m_connectTimeoutSec,
			0);

		client.set_read_timeout(
			m_readTimeoutSec,
			0);

		client.set_write_timeout(
			m_writeTimeoutSec,
			0);

		client.set_follow_location(
			true);

		return ConvertResponse(
			client.Post(
				requestPath.c_str(),
				requestHeaders,
				body,
				contentType.c_str()));
	}

	httplib::Client client(
		url.host,
		url.port);

	client.set_connection_timeout(
		m_connectTimeoutSec,
		0);

	client.set_read_timeout(
		m_readTimeoutSec,
		0);

	client.set_write_timeout(
		m_writeTimeoutSec,
		0);

	client.set_follow_location(
		true);

	return ConvertResponse(
		client.Post(
			requestPath.c_str(),
			requestHeaders,
			body,
			contentType.c_str()));
}