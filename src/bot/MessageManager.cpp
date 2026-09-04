#include "MessageManager.hpp"

#include "ConversationRegistry.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{
constexpr std::size_t MaxMessageLength = 3900;

void AssertIsApiAvailable(const TgBot::Api* api)
{
	if (api == nullptr)
	{
		throw std::invalid_argument("Клиент Telegram не может быть пустым");
	}
}

std::size_t FindSplitPosition(const std::string& text, const std::size_t from)
{
	const std::size_t limit = std::min(from + MaxMessageLength, text.size());
	if (limit == text.size())
	{
		return limit;
	}

	const std::size_t lineBreak = text.rfind('\n', limit);
	if (lineBreak != std::string::npos && lineBreak > from)
	{
		return lineBreak + 1;
	}

	std::size_t position = limit;
	while (position > from && (static_cast<unsigned char>(text[position]) & 0xC0) == 0x80)
	{
		--position;
	}

	return position;
}

std::vector<std::string> SplitText(const std::string& text)
{
	std::vector<std::string> parts;
	std::size_t offset = 0;

	while (offset < text.size())
	{
		const std::size_t next = FindSplitPosition(text, offset);
		parts.push_back(text.substr(offset, next - offset));
		offset = next;
	}

	return parts;
}
} // namespace

MessageManager::MessageManager(
	const TgBot::Api* api,
	ConversationRegistry& registry,
	const std::int64_t userId)
	: m_api(api)
	, m_registry(registry)
	, m_userId(userId)
{
	AssertIsApiAvailable(m_api);
}

void MessageManager::TrackMessage(const std::int64_t chatId, const std::int32_t messageId)
{
	m_registry.Track(m_userId, chatId, messageId);
}

std::int32_t MessageManager::SendStatus(const std::int64_t chatId, const std::string& statusText)
{
	const auto message = m_api->sendMessage(chatId, statusText);
	TrackMessage(chatId, message->messageId);

	return message->messageId;
}

void MessageManager::SendText(const std::int64_t chatId, const std::string& text)
{
	if (text.empty())
	{
		return;
	}

	for (const auto& part : SplitText(text))
	{
		const auto message = m_api->sendMessage(chatId, part);
		TrackMessage(chatId, message->messageId);
	}
}

std::int32_t MessageManager::SendWithKeyboard(
	const std::int64_t chatId,
	const std::string& text,
	const TgBot::InlineKeyboardMarkup::Ptr& keyboard)
{
	const auto message = m_api->sendMessage(chatId, text, nullptr, nullptr, keyboard);
	TrackMessage(chatId, message->messageId);

	return message->messageId;
}

void MessageManager::EditText(
	const std::int64_t chatId,
	const std::int32_t messageId,
	const std::string& text,
	const TgBot::InlineKeyboardMarkup::Ptr& keyboard)
{
	m_api->editMessageText(text, chatId, messageId, "", "", nullptr, keyboard);
}

void MessageManager::EditKeyboard(
	const std::int64_t chatId,
	const std::int32_t messageId,
	const TgBot::InlineKeyboardMarkup::Ptr& keyboard)
{
	m_api->editMessageReplyMarkup(chatId, messageId, "", keyboard);
}

void MessageManager::DeleteTrackedMessages(const std::int64_t chatId)
{
	for (const auto& reference : m_registry.TakeTrackedMessages(m_userId, chatId))
	{
		try
		{
			m_api->deleteMessage(reference.chatId, reference.messageId);
		}
		catch (const std::exception& error)
		{
			std::cerr << "Ошибка удаления сообщения: " << error.what() << std::endl;
		}
	}
}
