#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

namespace TgBot
{
class Api;
} // namespace TgBot

// RAII: пока объект жив, чат раз в несколько секунд получает статус "печатает",
// чтобы длинная обработка (1-5 минут) не выглядела зависшей.
class TypingIndicator
{
public:
	TypingIndicator(const TgBot::Api* api, std::int64_t chatId);
	~TypingIndicator();

	TypingIndicator(const TypingIndicator&) = delete;
	TypingIndicator& operator=(const TypingIndicator&) = delete;

private:
	void Run();

	const TgBot::Api* m_api;
	std::int64_t m_chatId;
	std::mutex m_mutex;
	std::condition_variable m_condition;
	bool m_stopping = false;
	std::thread m_thread;
};
