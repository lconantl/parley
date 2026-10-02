#pragma once

#include "ConversationController.hpp"

class ParleyBot
{
  public:
	ParleyBot(const Config& config, MatchStore& store, ITelegramClient& client);
	void Run();

  private:
	void Process(const nlohmann::json& update, const std::string& botId);
	MatchStore& m_store;
	ITelegramClient& m_client;
	ConversationController m_controller;
};
