#include "ParleyBot.hpp"

#include "http/TelegramError.hpp"
#include "system/StopSignal.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{
void AssertIsBotAvailable(const nlohmann::json& user)
{
	if (!user.is_object() || !user.value("is_bot", false) || !user.contains("id"))
	{
		throw std::runtime_error("Не удалось определить Telegram-бота по токену");
	}
}

nlohmann::json Commands()
{
	return {{"commands",
			 {{{"command", "start"}, {"description", "Начать"}},
			  {{"command", "feed"}, {"description", "Лента пар"}},
			  {{"command", "favorites"}, {"description", "Избранное"}},
			  {{"command", "progress"}, {"description", "Мой прогресс"}},
			  {{"command", "undo"}, {"description", "Отменить последнее решение"}},
			  {{"command", "help"}, {"description", "Как пользоваться"}}}}};
}

void LogError(const std::exception& error)
{
	std::cerr << "Ошибка бота: " << error.what() << std::endl;
}
}

ParleyBot::ParleyBot(const Config& config, MatchStore& store, ITelegramClient& client)
	: m_store(store), m_client(client), m_controller(config, store, client)
{
}

void ParleyBot::Process(const nlohmann::json& update, const std::string& botId)
{
	const auto updateId = update.at("update_id").get<std::int64_t>();
	if (updateId < m_store.GetOffset(botId))
	{
		return;
	}
	try
	{
		m_controller.Handle(update);
	}
	catch (const std::exception& error)
	{
		LogError(error);
		const auto message =
			update.value("message", update.value("callback_query", nlohmann::json::object())
										.value("message", nlohmann::json::object()));
		if (message.contains("chat") && message["chat"].value("type", "") == "private")
		{
			try
			{
				m_client.Call(
					"sendMessage",
					{{"chat_id", message["chat"]["id"]},
					 {"text",
					  "Не удалось завершить действие. Сохранённые пары доступны в избранном. "
					  "Откройте /feed или /favorites и попробуйте ещё раз."}});
			}
			catch (const std::exception& sendError)
			{
				LogError(sendError);
			}
		}
	}
	m_store.SetOffset(botId, updateId + 1);
}

void ParleyBot::Run()
{
	const auto user = m_client.Call("getMe", nlohmann::json::object());
	AssertIsBotAvailable(user);
	const auto botId = std::to_string(user.at("id").get<std::int64_t>());
	m_client.Call("deleteWebhook", {{"drop_pending_updates", false}});
	m_client.Call("setMyCommands", Commands());
	std::cout << "Бот запущен: @" << user.value("username", "") << std::endl;
	int failures = 0;
	while (!StopSignal::IsRequested())
	{
		try
		{
			const auto updates =
				m_client.Call("getUpdates", {{"offset", m_store.GetOffset(botId)},
											 {"timeout", 25},
											 {"limit", 20},
											 {"allowed_updates", {"message", "callback_query"}}});
			for (const auto& update : updates)
			{
				if (StopSignal::IsRequested())
				{
					break;
				}
				Process(update, botId);
			}
			failures = 0;
		}
		catch (const TelegramError& error)
		{
			if (error.GetCode() == 401 || error.GetCode() == 409)
			{
				throw;
			}
			LogError(error);
			StopSignal::Wait(std::chrono::seconds(std::min(++failures * 2, 30)));
		}
		catch (const std::exception& error)
		{
			if (!StopSignal::IsRequested())
			{
				LogError(error);
				StopSignal::Wait(std::chrono::seconds(std::min(++failures * 2, 30)));
			}
		}
	}
	std::cout << "Бот остановлен. Решения сохранены." << std::endl;
}
