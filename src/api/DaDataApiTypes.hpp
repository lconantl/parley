#pragma once

#include <nlohmann/json.hpp>
#include <string>

enum class DaDataHttpMethod
{
	Get,
	Post
};

enum class DaDataService
{
	Suggestions,
	Api,
	Profile
};

struct DaDataRequest
{
	DaDataService service;
	std::string endpoint;
	DaDataHttpMethod method;
	nlohmann::json body;
};

struct DaDataResponse
{
	long statusCode;
	nlohmann::json body;
};

struct DaDataApiStatistics
{
	std::size_t requestCount;
	std::size_t successfulRequestCount;
	std::size_t failedRequestCount;
};