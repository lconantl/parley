#pragma once

#include "IAIClient.hpp"
#include "config/Config.hpp"
#include "http/IHttpClient.hpp"

#include <memory>

class DeepSeekClient final : public IAIClient
{
public:
	DeepSeekClient(const Config& config, std::unique_ptr<IHttpClient> httpClient);
	~DeepSeekClient() override;

	std::string Complete(
		const std::string& systemPrompt,
		const std::string& userPrompt,
		bool jsonMode = false) override;

private:
	Config m_config;
	std::unique_ptr<IHttpClient> m_httpClient;
};