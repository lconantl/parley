#include "MessageParser.hpp"
#include <stdexcept>
#include <tgbot/tgbot.h>

namespace
{
constexpr char CommandPrefix = '/';
constexpr char MentionSeparator = '@';

void AssertIsMessageValid(const std::shared_ptr<TgBot::Message>& message)
{
	if (message == nullptr)
	{
		throw std::invalid_argument("Получено пустое сообщение");
	}

	if (message->chat == nullptr)
	{
		throw std::invalid_argument("Сообщение получено без информации о чате");
	}

	if (message->from == nullptr)
	{
		throw std::invalid_argument("Сообщение получено без информации об отправителе");
	}
}

std::string StripMention(const std::string& command)
{
	const std::size_t position = command.find(MentionSeparator);
	if (position == std::string::npos)
	{
		return command;
	}

	return command.substr(0, position);
}

std::string TrimSpaces(const std::string& text)
{
	const std::size_t begin = text.find_first_not_of(" \t\n\r");
	if (begin == std::string::npos)
	{
		return {};
	}

	const std::size_t end = text.find_last_not_of(" \t\n\r");

	return text.substr(begin, end - begin + 1);
}

bool IsCommand(const std::string& text)
{
	return !text.empty() && text.front() == CommandPrefix;
}

void ParseCommand(const std::string& text, ParsedMessage& parsed)
{
	const std::size_t separator = text.find(' ');

	if (separator == std::string::npos)
	{
		parsed.command = StripMention(text.substr(1));
		return;
	}

	parsed.command = StripMention(text.substr(1, separator - 1));
	parsed.arguments = TrimSpaces(text.substr(separator + 1));
}
} // namespace

ParsedMessage MessageParser::Parse(const std::shared_ptr<TgBot::Message>& rawMessage)
{
	AssertIsMessageValid(rawMessage);

	ParsedMessage parsed;
	parsed.userId = rawMessage->from->id;
	parsed.chatId = rawMessage->chat->id;
	parsed.messageId = rawMessage->messageId;
	parsed.text = TrimSpaces(rawMessage->text);

	if (IsCommand(parsed.text))
	{
		ParseCommand(parsed.text, parsed);
	}

	return parsed;
}