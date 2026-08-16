#pragma once

#include "config/Config.hpp"
#include <tgbot/tgbot.h>

class ParleyBot
{
public:
	explicit ParleyBot(const Config& config);

	void Run() const;

private:
	void SetupHandlers();
	void HandleMessage(const TgBot::Message::Ptr& message) const;
	[[nodiscard]] bool IsUserAllowed(int64_t userId) const;

	TgBot::Bot m_bot;
	const Config& m_config;
};