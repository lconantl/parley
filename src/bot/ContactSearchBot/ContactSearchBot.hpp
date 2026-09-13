#pragma once

#include "application/search/ContactSearchService/ContactSearchService.hpp"
#include "bot/ChatSequencer/ChatSequencer.hpp"
#include "bot/access/AccessPolicy.hpp"
#include "infrastructure/pool/WorkerPool.hpp"

#include <memory>
#include <string>
#include <tgbot/tgbot.h>

class ContactSearchBot
{
public:
	ContactSearchBot(std::string token, AccessPolicy accessPolicy, std::shared_ptr<ContactSearchService> searchService);
	~ContactSearchBot();

	ContactSearchBot(const ContactSearchBot&) = delete;
	ContactSearchBot& operator=(const ContactSearchBot&) = delete;

	void Run() const;

private:
	void SubscribeToMessages();
	void HandleMessage(const TgBot::Message::Ptr& message);
	void HandleStart(std::int64_t chatId) const;
	void HandleQuery(std::int64_t chatId, const std::string& text) const;

	TgBot::Bot m_bot;
	AccessPolicy m_accessPolicy;
	std::shared_ptr<ContactSearchService> m_searchService;
	mutable WorkerPool m_workers;
	mutable ChatSequencer m_sequencer;
};
