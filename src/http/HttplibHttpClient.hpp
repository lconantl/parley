#pragma once

#include "IHttpClient.hpp"

class HttplibHttpClient final : public IHttpClient
{
public:
	HttplibHttpClient();
	~HttplibHttpClient() override;

	HttpResponse Post(
		const std::string& url,
		const std::vector<HttpHeader>& headers,
		const std::string& body) override;
};