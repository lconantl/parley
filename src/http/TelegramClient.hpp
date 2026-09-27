#pragma once

#include "bot/ITelegramClient.hpp"
#include <chrono>
#include <cstdint>
#include <map>

class TelegramClient : public ITelegramClient
{
  public:
	explicit TelegramClient(std::string token);
	nlohmann::json Call(const std::string& method, const nlohmann::json& parameters) override;

  private:
	std::string m_token;
	std::map<std::int64_t, std::chrono::steady_clock::time_point> m_nextSend;
};
