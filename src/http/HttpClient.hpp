#pragma once

#include <string>
#include <unordered_map>

struct HttpResponse
{
	int status = 0;
	std::string body;
	std::unordered_map<std::string, std::string> headers;
};

class HttpClient
{
public:
	HttpClient(
		std::string baseUrl,
		long connectTimeoutSec,
		long readTimeoutSec,
		long writeTimeoutSec,
		std::string userAgent);

	[[nodiscard]]
	HttpResponse Get(
		const std::string& path,
		const std::unordered_map<std::string, std::string>& headers = {}) const;

	[[nodiscard]]
	HttpResponse Post(
		const std::string& path,
		const std::string& body,
		const std::string& contentType,
		const std::unordered_map<std::string, std::string>& headers = {}) const;

private:
	std::string m_baseUrl;
	long m_connectTimeoutSec;
	long m_readTimeoutSec;
	long m_writeTimeoutSec;
	std::string m_userAgent;
};