#include "HttplibHttpClient.hpp"
#include <httplib.h>
#include <regex>
#include <stdexcept>

namespace
{
struct ParsedUrl
{
	std::string host;
	std::string path;
};

void AssertIsPatternMatched(const bool isMatched)
{
	if (!isMatched)
	{
		throw std::runtime_error("Неверный формат URL для HTTP клиента");
	}
}

ParsedUrl ParseUrl(const std::string& url)
{
	const std::regex urlRegex(R"(^(https?://[^/]+)(/.*)?$)");
	std::smatch match;
	AssertIsPatternMatched(std::regex_match(url, match, urlRegex));

	ParsedUrl parsed;
	parsed.host = match[1].str();
	parsed.path = match[2].matched ? match[2].str() : "/";
	return parsed;
}

httplib::Headers ConvertHeaders(const std::vector<HttpHeader>& customHeaders)
{
	httplib::Headers libHeaders;
	for (const auto& header : customHeaders)
	{
		libHeaders.emplace(header.key, header.value);
	}
	return libHeaders;
}

void AssertIsRequestSuccessful(const httplib::Result& result)
{
	if (!result)
	{
		const std::string errorMessage = "Сетевая ошибка при запросе: " + to_string(result.error());
		throw std::runtime_error(errorMessage);
	}
}
} // namespace

HttplibHttpClient::HttplibHttpClient()
{
}

HttplibHttpClient::~HttplibHttpClient()
{
}

HttpResponse HttplibHttpClient::Post(
	const std::string& url,
	const std::vector<HttpHeader>& headers,
	const std::string& body)
{
	const ParsedUrl parsed = ParseUrl(url);
	httplib::Client client(parsed.host);

	client.set_connection_timeout(10, 0);
	client.set_read_timeout(120, 0);

	const httplib::Headers libHeaders = ConvertHeaders(headers);
	const std::string contentType = "application/json";

	const httplib::Result result = client.Post(parsed.path, libHeaders, body, contentType);
	AssertIsRequestSuccessful(result);

	HttpResponse response;
	response.statusCode = result->status;
	response.body = result->body;

	return response;
}