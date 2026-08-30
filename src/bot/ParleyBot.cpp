#include "ParleyBot.hpp"
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <tgbot/tgbot.h>

namespace
{
void AssertIsFilledToken(const std::string& token)
{
	if (token.empty())
	{
		throw std::invalid_argument("Токен бота не задан");
	}
}

void AssertIsChatAvailable(const TgBot::Message& message)
{
	if (message.chat == nullptr)
	{
		throw std::runtime_error("Сообщение получено без информации о чате");
	}
}

std::unique_ptr<TgBot::Bot> CreateBot(const std::string& token)
{
	AssertIsFilledToken(token);
	return std::make_unique<TgBot::Bot>(token);
}

std::string BuildDescriptionText()
{
	return "Привет, я бот <b>Parley</b> для анализа компаний."
		   "Чтобы начать анализ - пришли ИНН компании и её название.\n"
		   "\n"
		   "Доступны следующие команды:\n"
		   "/report [ИНН] - получить отчёт\n"
		   "/pres [ИНН] - получить презентацию\n"
		   "/pay - оплатить API для AI анализа";
}

std::string BuildStubText()
{
	return "В разработке 🛠️";
}

std::vector<std::string> BuildStubCommandNames()
{
	return {"report", "pres", "pay"};
}

void LogError(const std::exception& error)
{
	std::cerr << "Ошибка бота: " << error.what() << std::endl;
}

void RunProtected(const std::function<void()>& action)
{
	try
	{
		action();
	}
	catch (const std::exception& error)
	{
		LogError(error);
	}
}

void SendHtmlText(const TgBot::Api& api, const TgBot::Message& message, const std::string& text)
{
	AssertIsChatAvailable(message);
	api.sendMessage(message.chat->id, text, nullptr, nullptr, nullptr, "HTML");
}

void RegisterStartCommand(TgBot::Bot& bot)
{
	bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
		RunProtected([&bot, message] {
			SendHtmlText(bot.getApi(), *message, BuildDescriptionText());
		});
	});
}

void RegisterStubCommand(TgBot::Bot& bot, const std::string& commandName)
{
	bot.getEvents().onCommand(commandName, [&bot](TgBot::Message::Ptr message) {
		RunProtected([&bot, message] {
			SendHtmlText(bot.getApi(), *message, BuildStubText());
		});
	});
}

void RegisterStubCommands(TgBot::Bot& bot)
{
	for (const auto& commandName : BuildStubCommandNames())
	{
		RegisterStubCommand(bot, commandName);
	}
}

void RegisterPlainTextStub(TgBot::Bot& bot)
{
	bot.getEvents().onNonCommandMessage([&bot](TgBot::Message::Ptr message) {
		RunProtected([&bot, message] {
			SendHtmlText(bot.getApi(), *message, BuildStubText());
		});
	});
}

void RegisterHandlers(TgBot::Bot& bot)
{
	RegisterStartCommand(bot);
	RegisterStubCommands(bot);
	RegisterPlainTextStub(bot);
}

std::shared_ptr<TgBot::BotCommand> MakeMenuItem(const std::string& commandName,
	const std::string& description)
{
	auto menuItem = std::make_shared<TgBot::BotCommand>();
	menuItem->command = commandName;
	menuItem->description = description;
	return menuItem;
}

void PublishCommandMenu(const TgBot::Api& api)
{
	api.setMyCommands({
		MakeMenuItem("start", "Описание бота"),
		MakeMenuItem("report", "Получить отчёт по ИНН"),
		MakeMenuItem("pres", "Получить презентацию по ИНН"),
		MakeMenuItem("pay", "Оплатить API для AI анализа"),
	});
}

void LogStartup(const TgBot::Api& api)
{
	std::cout << "Бот запущен: " << api.getMe()->username << std::endl;
}
} // namespace

ParleyBot::ParleyBot(const std::string& token)
	: m_bot(CreateBot(token))
{
	RegisterHandlers(*m_bot);
}

ParleyBot::~ParleyBot() = default;

void ParleyBot::Run() const
{
	m_bot->getApi().deleteWebhook();
	PublishCommandMenu(m_bot->getApi());
	LogStartup(m_bot->getApi());

	TgBot::TgLongPoll longPoll(*m_bot);
	while (true)
	{
		RunProtected([&longPoll] { longPoll.start(); });
	}
}