#pragma once

#include "http/HttpClient.hpp"

#include <nlohmann/json_fwd.hpp>

#include <string>

class Config;

class PolzaAiClient
{
public:
	explicit PolzaAiClient(
		const Config& config);

	[[nodiscard]]
	nlohmann::json CompleteJson(
		const std::string& systemPrompt,
		const std::string& userPrompt) const;

	[[nodiscard]]
	const std::string& GetModel() const;

private:
	HttpClient m_httpClient;
	std::string m_apiKey;
	std::string m_model;
};
