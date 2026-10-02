#include "MessageManager.hpp"

#include "http/TelegramError.hpp"
#include "ui/Text.hpp"

namespace
{
nlohmann::json Parameters(std::int64_t chatId, const std::string& text,
						  const nlohmann::json& keyboard, bool html)
{
	nlohmann::json parameters = {
		{"chat_id", chatId}, {"text", text}, {"link_preview_options", {{"is_disabled", true}}}};
	if (!keyboard.empty())
	{
		parameters["reply_markup"] = keyboard;
	}
	if (html)
	{
		parameters["parse_mode"] = "HTML";
	}
	return parameters;
}
}

MessageManager::MessageManager(ITelegramClient& client) : m_client(client)
{
}

void MessageManager::Send(std::int64_t chatId, const std::string& text,
						  const nlohmann::json& keyboard, bool html)
{
	const auto parts = html ? std::vector<std::string>{text} : Text::Split(text);
	for (std::size_t index = 0; index < parts.size(); ++index)
	{
		m_client.Call("sendMessage",
					  Parameters(chatId, parts[index],
								 index + 1 == parts.size() ? keyboard : nlohmann::json{}, html));
	}
}

void MessageManager::Show(std::int64_t chatId, std::int64_t messageId, const std::string& text,
						  const nlohmann::json& keyboard, bool html)
{
	if (messageId == 0)
	{
		Send(chatId, text, keyboard, html);
		return;
	}
	auto parameters = Parameters(chatId, text, keyboard, html);
	parameters["message_id"] = messageId;
	try
	{
		m_client.Call("editMessageText", parameters);
	}
	catch (const TelegramError& error)
	{
		if (error.IsUnchanged())
		{
			return;
		}
		if (!error.IsMissingMessage())
		{
			throw;
		}
		Send(chatId, text, keyboard, html);
	}
}

void MessageManager::Acknowledge(const std::string& callbackId, const std::string& text)
{
	try
	{
		m_client.Call("answerCallbackQuery", {{"callback_query_id", callbackId}, {"text", text}});
	}
	catch (const TelegramError& error)
	{
		if (error.GetCode() != 400)
		{
			throw;
		}
	}
}
