#pragma once

#include "HttpResponse.hpp"

#include <string>

using CURL = void;
struct curl_slist;

class CurlSession
{
public:
	CurlSession();
	~CurlSession();

	CurlSession(const CurlSession&) = delete;
	CurlSession& operator=(const CurlSession&) = delete;
	CurlSession(CurlSession&&) = delete;
	CurlSession& operator=(CurlSession&&) = delete;

	void SetUrl(const std::string& url) const;
	void SetTimeout(long seconds) const;
	void AddHeader(const std::string& header);
	void SetPostBody(std::string body);

	HttpResponse Perform() const;

private:
	CURL* m_handle;
	curl_slist* m_headers;
	std::string m_postBody;
};