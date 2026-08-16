#include "ParleyBot.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

ParleyBot::ParleyBot(const Config& config)
	: m_bot(config.GetBotToken())
	, m_config(config)
{
	SetupHandlers();
}

void ParleyBot::Run() const
{
	try
	{
		TgBot::TgLongPoll longPoll(m_bot);

		while (true)
		{
			longPoll.start();
		}
	}
	catch (const TgBot::TgException& exception)
	{
		throw std::runtime_error(std::string("[Telegram]: ") + exception.what());
	}
}

void ParleyBot::SetupHandlers()
{
	m_bot.getEvents().onAnyMessage([this](const TgBot::Message::Ptr& message) {
		HandleMessage(message);
	});
}

void ParleyBot::HandleMessage(const TgBot::Message::Ptr& message) const
{
	std::cout << "Received message from user ID: " << message->from->id << std::endl;

	if (IsUserAllowed(message->from->id))
	{
		m_bot.getApi().sendMessage(message->chat->id, "hello world");
		std::cout << "Sent 'hello world' to " << message->from->username << std::endl;
	}
	else
	{
		std::cout << "User " << message->from->id << " is not allowed." << std::endl;
	}
}

bool ParleyBot::IsUserAllowed(const int64_t userId) const
{
	const auto& allowedUsers = m_config.GetAllowedUsers();
	const std::string userIdStr = std::to_string(userId);

	return std::ranges::find(allowedUsers, userIdStr) != allowedUsers.end();
}