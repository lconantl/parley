#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

enum class CheckoHttpMethod
{
	Get,
	Post
};

struct CheckoRequest
{
	std::string endpoint;
	CheckoHttpMethod method;
	std::unordered_map<std::string, std::string> parameters;
};

struct CheckoResponse
{
	long statusCode;
	nlohmann::json body;
};

struct CheckoApiStatistics
{
	std::size_t requestCount;
	std::size_t successfulRequestCount;
	std::size_t failedRequestCount;
};