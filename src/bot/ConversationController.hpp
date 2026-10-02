#pragma once

#include "MessageManager.hpp"
#include "config/Config.hpp"
#include "storage/MatchStore.hpp"

class ConversationController
{
  public:
	ConversationController(const Config& config, MatchStore& store, ITelegramClient& client);
	void Handle(const nlohmann::json& update);

  private:
	void HandleMessage(const nlohmann::json& message, std::int64_t updateId);
	void HandleCallback(const nlohmann::json& query, std::int64_t updateId);
	void ShowFeed(std::int64_t userId, std::int64_t messageId = 0);
	void ShowCard(std::int64_t userId, const Match& match, std::int64_t messageId = 0);
	void ShowFavorites(std::int64_t userId, int offset, std::int64_t messageId = 0);
	void ShowDraft(std::int64_t userId, const Match& match, const std::string& code);
	void ShowTexts(std::int64_t userId, const Match& match, std::int64_t messageId);
	void HandleMatchAction(std::int64_t userId, std::int64_t messageId, std::int64_t updateId,
						   const std::string& action, const Match& match);

	const Config& m_config;
	MatchStore& m_store;
	MessageManager m_messages;
};
