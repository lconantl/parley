#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

struct PolzaAnswer
{
	std::string content;
	double costRub = 0.0;
};

class PolzaClient
{
public:
	PolzaClient(std::string baseUrl, std::string apiKey, std::string model);
	~PolzaClient();

	PolzaAnswer AskWithSearch(
		const std::string& systemPrompt,
		const std::string& userPrompt,
		std::size_t maxSearchResults) const;

	PolzaAnswer AskStructured(
		const std::string& systemPrompt,
		const std::string& userPrompt,
		const std::string& schemaName,
		const nlohmann::json& schema) const;

	double GetTotalCost() const;

private:
	class Impl;

	std::unique_ptr<Impl> m_impl;
};