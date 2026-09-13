#pragma once

#include <string>

enum class ChannelType
{
	Phone,
	TelegramUsername,
	TelegramId,
	Email
};

struct ContactChannel
{
	ChannelType type = ChannelType::Phone;
	std::string value;
};
