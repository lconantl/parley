#pragma once

#include <cstdint>
#include <string>
#include <tgbot/tgbot.h>

class ConversationRegistry;

// Все методы, кроме DeleteTrackedMessages, отправляют/редактируют эфемерные сообщения:
// они автоматически попадают в список на удаление конкретного пользователя в
// ConversationRegistry. Готовые документы через этот класс не отправляются — см. ArtifactSender.
class MessageManager
{
public:
	MessageManager(const TgBot::Api* api, ConversationRegistry& registry, std::int64_t userId);

	void TrackMessage(std::int64_t chatId, std::int32_t messageId);
	std::int32_t SendStatus(std::int64_t chatId, const std::string& statusText);
	void SendText(std::int64_t chatId, const std::string& text);
	std::int32_t SendWithKeyboard(
		std::int64_t chatId,
		const std::string& text,
		const TgBot::InlineKeyboardMarkup::Ptr& keyboard);
	void EditText(
		std::int64_t chatId,
		std::int32_t messageId,
		const std::string& text,
		const TgBot::InlineKeyboardMarkup::Ptr& keyboard);
	void EditKeyboard(
		std::int64_t chatId,
		std::int32_t messageId,
		const TgBot::InlineKeyboardMarkup::Ptr& keyboard);
	void DeleteTrackedMessages(std::int64_t chatId);

private:
	const TgBot::Api* m_api;
	ConversationRegistry& m_registry;
	std::int64_t m_userId;
};
