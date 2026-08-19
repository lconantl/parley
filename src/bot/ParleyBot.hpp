#pragma once

#include "ai/PolzaAiClient.hpp"
#include "bfo/BfoWebScraper.hpp"
#include <string>
#include <tgbot/tgbot.h>

class ParleyBot
{
public:
	explicit ParleyBot(const Config& config);

	void Run() const;

private:
	void SetupHandlers();
	void HandleMessage(const TgBot::Message::Ptr& message) const;

	[[nodiscard]]
	bool IsUserAllowed(int64_t userId) const;
	void HandleBfoSearch(int64_t chatId, const std::string& query) const;
	void SendCompanyProfile(int64_t chatId, int organizationId) const;
	void HandleDueDiligenceCommand(int64_t chatId, const std::string& query) const;

	[[nodiscard]]
	static bool IsCommand(const std::string& text);

	[[nodiscard]]
	static std::string ExtractCommandArgument(const std::string& text, const std::string& command);

	TgBot::Bot m_bot;
	const Config& m_config;
	BfoWebScraper m_scraper;
	PolzaAiClient m_polzaClient;
};