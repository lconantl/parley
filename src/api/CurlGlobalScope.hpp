#pragma once

class CurlGlobalScope
{
public:
	CurlGlobalScope();
	~CurlGlobalScope();

	CurlGlobalScope(const CurlGlobalScope&) = delete;
	CurlGlobalScope& operator=(const CurlGlobalScope&) = delete;
	CurlGlobalScope(CurlGlobalScope&&) = delete;
	CurlGlobalScope& operator=(CurlGlobalScope&&) = delete;
};