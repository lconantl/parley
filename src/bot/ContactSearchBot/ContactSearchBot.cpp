#include "ContactSearchBot.hpp"
#include "application/search/ContactResultFormatter/ContactResultFormatter.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
constexpr auto WelcomeText =
	"Привет! Я ищу людей по базе контактов. Опишите, кого ищете, обычным языком — например: "
	"«найди ментора для Степана, программиста из Ижкара».";
constexpr auto StartCommand = "/start";

std::string RequireToken(std::string token)
{
	if (token.empty())
	{
		throw std::invalid_argument("Токен телеграм-бота не может быть пустым");
	}

	return token;
}
} // namespace

ContactSearchBot::ContactSearchBot(
	std::string token,
	AccessPolicy accessPolicy,
	std::shared_ptr<ContactSearchService> searchService)
	: m_bot(RequireToken(std::move(token)))
	, m_accessPolicy(std::move(accessPolicy))
	, m_searchService(std::move(searchService))
	, m_workers(WorkerPool::SuggestThreadCount())
{
	SubscribeToMessages();
}

ContactSearchBot::~ContactSearchBot() = default;

void ContactSearchBot::SubscribeToMessages()
{
	m_bot.getEvents().onAnyMessage([this](const TgBot::Message::Ptr& message) {
		HandleMessage(message);
	});
}

void ContactSearchBot::HandleMessage(const TgBot::Message::Ptr& message)
{
	if (!message || !message->from || !message->chat)
	{
		return;
	}

	if (!m_accessPolicy.IsAllowed(message->from->id))
	{
		return;
	}

	const std::int64_t chatId = message->chat->id;

	if (message->text == StartCommand)
	{
		HandleStart(chatId);
		return;
	}

	if (message->text.empty())
	{
		return;
	}

	HandleQuery(chatId, message->text);
}

void ContactSearchBot::HandleStart(const std::int64_t chatId) const
{
	m_bot.getApi().sendMessage(chatId, WelcomeText);
}

void ContactSearchBot::HandleQuery(const std::int64_t chatId, const std::string& text) const
{
	const TgBot::Api* api = &m_bot.getApi();
	const std::shared_ptr<ContactSearchService> searchService = m_searchService;

	m_workers.Post([api, searchService, chatId, text] {
		try
		{
			const std::vector<SearchResult> results = searchService->Search(text);
			api->sendMessage(chatId, ContactResultFormatter::Format(results));
		}
		catch (const std::exception& exception)
		{
			std::cerr << "[ContactSearchBot] Ошибка обработки запроса: " << exception.what() << std::endl;
			api->sendMessage(chatId, "Не получилось обработать запрос, попробуйте ещё раз.");
		}
	});
}

void ContactSearchBot::Run() const
{
	try
	{
		TgBot::TgLongPoll longPoll(m_bot);
		while (true)
		{
			longPoll.start();
		}
	}
	catch (const TgBot::TgException& exception)
	{
		throw std::runtime_error(std::string("[Telegram]: ") + exception.what());
	}
}
