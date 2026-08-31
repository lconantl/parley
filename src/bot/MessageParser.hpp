#pragma once

#include <cstdint>
#include <string>
#include <tgbot/types/Message.h>

struct ParsedMessage
{
	int64_t userId;
	int64_t chatId;
	int32_t messageId;
	std::string text;
	std::string command;
	std::string arguments;
};

class MessageParser
{
public:
	static ParsedMessage Parse(const std::shared_ptr<TgBot::Message>& rawMessage);
};