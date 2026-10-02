#pragma once

#include <nlohmann/json.hpp>
#include <string>

class ITelegramClient
{
  public:
	virtual ~ITelegramClient();
	virtual nlohmann::json Call(const std::string& method, const nlohmann::json& parameters) = 0;
};
