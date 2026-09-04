#include "TypingIndicator.hpp"

#include <chrono>
#include <exception>
#include <stdexcept>
#include <tgbot/tgbot.h>

namespace
{
constexpr auto TypingAction = "typing";
constexpr std::chrono::seconds RepeatInterval{4};

void AssertIsApiAvailable(const TgBot::Api* api)
{
	if (api == nullptr)
	{
		throw std::invalid_argument("Клиент Telegram не может быть пустым");
	}
}
} // namespace

TypingIndicator::TypingIndicator(const TgBot::Api* api, const std::int64_t chatId)
	: m_api(api)
	, m_chatId(chatId)
{
	AssertIsApiAvailable(m_api);

	m_api->sendChatAction(m_chatId, TypingAction);
	m_thread = std::thread([this] { Run(); });
}

TypingIndicator::~TypingIndicator()
{
	{
		std::lock_guard lock(m_mutex);
		m_stopping = true;
	}

	m_condition.notify_all();

	if (m_thread.joinable())
	{
		m_thread.join();
	}
}

void TypingIndicator::Run()
{
	std::unique_lock lock(m_mutex);

	while (!m_condition.wait_for(lock, RepeatInterval, [this] { return m_stopping; }))
	{
		lock.unlock();

		try
		{
			m_api->sendChatAction(m_chatId, TypingAction);
		}
		catch (const std::exception&)
		{
			// сеть могла моргнуть — повторим на следующем цикле
		}

		lock.lock();
	}
}
