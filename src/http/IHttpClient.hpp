#pragma once

#include "HttpHeader.hpp"
#include "HttpResponse.hpp"
#include <string>
#include <vector>

class IHttpClient
{
public:
	virtual ~IHttpClient() = default;

	virtual HttpResponse Post(
		const std::string& url,
		const std::vector<HttpHeader>& headers,
		const std::string& body) = 0;
};