#pragma once

#include "ITelegramClient.hpp"
#include <cstdint>

class MessageManager
{
  public:
	explicit MessageManager(ITelegramClient& client);
	void Send(std::int64_t chatId, const std::string& text, const nlohmann::json& keyboard = {},
			  bool html = false);
	void Show(std::int64_t chatId, std::int64_t messageId, const std::string& text,
			  const nlohmann::json& keyboard, bool html = true);
	void Acknowledge(const std::string& callbackId, const std::string& text = {});

  private:
	ITelegramClient& m_client;
};
