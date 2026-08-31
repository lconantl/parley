#include "MessageManager.hpp"
#include <tgbot/tgbot.h>
#include <iostream>

MessageManager::MessageManager(TgBot::Api* api)
	: m_api(api)
{
}

void MessageManager::TrackMessage(int64_t chatId, int32_t messageId)
{
	m_trackedMessages.push_back({chatId, messageId});
}

void MessageManager::SendStatus(int64_t chatId, const std::string& statusText)
{
	if (m_api)
	{
		auto msg = m_api->sendMessage(chatId, statusText);
		TrackMessage(chatId, msg->messageId);
	}
}

void MessageManager::DeleteTrackedMessages()
{
	if (!m_api)
	{
		return;
	}

	for (const auto& ref : m_trackedMessages)
	{
		try
		{
			m_api->deleteMessage(ref.chatId, ref.messageId);
		}
		catch (const std::exception& error)
		{
			std::cerr << "Ошибка удаления сообщения: " << error.what() << std::endl;
		}
	}
	m_trackedMessages.clear();
}