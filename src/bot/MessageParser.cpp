#include "MessageParser.hpp"
#include <stdexcept>
#include <tgbot/tgbot.h>

namespace
{
void AssertIsMessageValid(const std::shared_ptr<TgBot::Message>& message)
{
	if (!message)
	{
		throw std::invalid_argument("Получено пустое сообщение");
	}
	if (!message->chat)
	{
		throw std::invalid_argument("Сообщение получено без информации о чате");
	}
	if (!message->from)
	{
		throw std::invalid_argument("Сообщение получено без информации об отправителе");
	}
}
} // namespace

ParsedMessage MessageParser::Parse(const std::shared_ptr<TgBot::Message>& rawMessage)
{
	AssertIsMessageValid(rawMessage);

	ParsedMessage parsed;
	parsed.userId = rawMessage->from->id;
	parsed.chatId = rawMessage->chat->id;
	parsed.messageId = rawMessage->messageId;
	parsed.text = rawMessage->text;

	if (!rawMessage->text.empty() && rawMessage->text[0] == '/')
	{
		auto spacePos = rawMessage->text.find(' ');
		if (spacePos != std::string::npos)
		{
			parsed.command = rawMessage->text.substr(1, spacePos - 1);
			parsed.arguments = rawMessage->text.substr(spacePos + 1);
		}
		else
		{
			parsed.command = rawMessage->text.substr(1);
		}
	}

	return parsed;
}