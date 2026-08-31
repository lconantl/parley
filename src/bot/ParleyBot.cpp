#include "ParleyBot.hpp"
#include "MessageParser.hpp"
#include "handlers/MarkdownReportCommandHandler.hpp"
#include "handlers/StartCommandHandler.hpp"
#include "handlers/StubCommandHandler.hpp"
#include <iostream>
#include <stdexcept>
#include <tgbot/tgbot.h>
#include <utility>

namespace
{
constexpr auto BusyText = "Предыдущий запрос еще выполняется, дождитесь ответа";
constexpr auto FailureText = "Не удалось выполнить запрос";
constexpr auto DevelopmentText = "В разработке";

void AssertIsTokenValid(const std::string& token)
{
	if (token.empty())
	{
		throw std::invalid_argument("Токен бота не задан");
	}
}

std::unique_ptr<TgBot::Bot> CreateBot(const std::string& token)
{
	AssertIsTokenValid(token);

	return std::make_unique<TgBot::Bot>(token);
}

void LogError(const std::exception& error)
{
	std::cerr << "Ошибка бота: " << error.what() << std::endl;
}

std::shared_ptr<TgBot::BotCommand> MakeMenuItem(const CommandInfo& command)
{
	auto menuItem = std::make_shared<TgBot::BotCommand>();
	menuItem->command = command.name;
	menuItem->description = command.description;

	return menuItem;
}
} // namespace

ParleyBot::ParleyBot(
	const Config& config,
	std::shared_ptr<CompanyAnalyticsViewModel> analyticsViewModel,
	MetricFormatOptions formatOptions,
	std::filesystem::path outputDirectory)
	: m_bot(CreateBot(config.GetBotToken()))
	, m_accessPolicy(config.GetAllowedUsers())
{
	RegisterCommands(
		std::move(analyticsViewModel),
		std::move(formatOptions),
		std::move(outputDirectory));

	SubscribeToMessages();
}

ParleyBot::~ParleyBot() = default;

void ParleyBot::RegisterCommands(
	std::shared_ptr<CompanyAnalyticsViewModel> analyticsViewModel,
	MetricFormatOptions formatOptions,
	std::filesystem::path outputDirectory)
{
	auto report = std::make_shared<MarkdownReportCommandHandler>(
		std::move(analyticsViewModel),
		std::move(outputDirectory),
		std::move(formatOptions));

	m_router.Register(std::make_shared<StartCommandHandler>());
	m_router.Register(report);
	m_router.Register(std::make_shared<StubCommandHandler>(
		"pres", "Получить презентацию по ИНН", DevelopmentText));
	m_router.Register(std::make_shared<StubCommandHandler>(
		"pay", "Оплатить анализ", DevelopmentText));

	m_router.SetFallback(report);
}

void ParleyBot::SubscribeToMessages()
{
	m_bot->getEvents().onAnyMessage([this](TgBot::Message::Ptr message) {
		HandleMessage(message);
	});
}

void ParleyBot::HandleMessage(const std::shared_ptr<TgBot::Message>& rawMessage) const
{
	try
	{
		const ParsedMessage message = MessageParser::Parse(rawMessage);

		if (!m_accessPolicy.IsAllowed(message.userId))
		{
			return;
		}

		HandleSession(message);
	}
	catch (const std::exception& error)
	{
		LogError(error);
	}
}

void ParleyBot::HandleSession(const ParsedMessage& message) const
{
	MessageManager messages(&m_bot->getApi());

	const SessionLock lock(m_sessions, message.userId);
	if (!lock.IsAcquired())
	{
		messages.SendText(message.chatId, BusyText);
		return;
	}

	messages.TrackMessage(message.chatId, message.messageId);

	try
	{
		m_router.Dispatch({message, &messages});
	}
	catch (const std::exception& error)
	{
		LogError(error);
		messages.SendText(message.chatId, std::string(FailureText) + ": " + error.what());
	}

	messages.DeleteTrackedMessages();
}

void ParleyBot::PublishCommandMenu() const
{
	std::vector<std::shared_ptr<TgBot::BotCommand>> menu;

	for (const auto& command : m_router.ListCommands())
	{
		menu.push_back(MakeMenuItem(command));
	}

	m_bot->getApi().setMyCommands(menu);
}

void ParleyBot::Run() const
{
	PublishCommandMenu();
	std::cout << "Бот запущен: " << m_bot->getApi().getMe()->username << std::endl;

	TgBot::TgLongPoll longPoll(*m_bot);

	while (true)
	{
		try
		{
			longPoll.start();
		}
		catch (const std::exception& error)
		{
			LogError(error);
		}
	}
}