#pragma once

#include "ParsedMessage.hpp"
#include <memory>

namespace TgBot
{
class Message;
} // namespace TgBot

class MessageParser
{
public:
	static ParsedMessage Parse(const std::shared_ptr<TgBot::Message>& rawMessage);
};