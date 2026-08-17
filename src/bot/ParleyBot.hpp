#pragma once

#include "bfo/BfoWebScraper.hpp"
#include "config/Config.hpp"

#include <tgbot/tgbot.h>

#include <cstdint>
#include <string>

class ParleyBot
{
public:
	explicit ParleyBot(
		const Config& config);

	void Run() const;

private:
	void SetupHandlers();

	void HandleMessage(
		const TgBot::Message::Ptr& message) const;

	[[nodiscard]]
	bool IsUserAllowed(
		int64_t userId) const;

	void HandleBfoSearch(
		int64_t chatId,
		const std::string& query) const;

	void SendCompanyProfile(
		int64_t chatId,
		int organizationId) const;

	[[nodiscard]]
	static bool IsCommand(
		const std::string& text);

	TgBot::Bot m_bot;
	const Config& m_config;
	BfoWebScraper m_scraper;
};