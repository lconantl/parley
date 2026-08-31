#pragma once

#include <cstdint>
#include <string>

struct ParsedMessage
{
	std::int64_t userId = 0;
	std::int64_t chatId = 0;
	std::int32_t messageId = 0;
	std::string text;
	std::string command;
	std::string arguments;
};