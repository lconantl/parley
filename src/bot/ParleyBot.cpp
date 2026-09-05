#include "ParleyBot.hpp"

#include "AccessPolicy.hpp"
#include "documents/OnePagerDocumentBuilder.hpp"
#include "documents/PresentationDocumentBuilder.hpp"
#include "documents/ReportDocumentBuilder.hpp"

#include <iostream>
#include <stdexcept>
#include <tgbot/tgbot.h>
#include <utility>

namespace
{
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

ConversationController::Dependencies BuildControllerDependencies(ParleyBot::Dependencies dependencies)
{
	ConversationController::Dependencies controllerDependencies;
	controllerDependencies.analyticsViewModel = dependencies.analyticsViewModel;
	controllerDependencies.reportBuilder = std::make_shared<ReportDocumentBuilder>(
		dependencies.outputDirectory,
		dependencies.formatOptions);
	controllerDependencies.presentationBuilder = std::make_shared<PresentationDocumentBuilder>(
		std::move(dependencies.narrator),
		dependencies.outputDirectory,
		dependencies.theme,
		dependencies.formatOptions);
	controllerDependencies.onePagerBuilder = std::make_shared<OnePagerDocumentBuilder>(
		std::move(dependencies.onePagerNarrator),
		dependencies.outputDirectory,
		std::move(dependencies.theme),
		dependencies.formatOptions);
	controllerDependencies.showSourceNotes = dependencies.showSourceNotes;
	controllerDependencies.author = dependencies.author;

	return controllerDependencies;
}
} // namespace

ParleyBot::ParleyBot(const Config& config, Dependencies dependencies)
	: m_bot(CreateBot(config.GetBotToken()))
	, m_workers(WorkerPool::SuggestThreadCount())
	, m_controller(std::make_unique<ConversationController>(
		  *m_bot,
		  AccessPolicy(config.GetAllowedUsers()),
		  BuildControllerDependencies(std::move(dependencies)),
		  m_workers))
{
	SubscribeToMessages();
}

ParleyBot::~ParleyBot() = default;

void ParleyBot::SubscribeToMessages()
{
	m_bot->getEvents().onAnyMessage([this](TgBot::Message::Ptr message) {
		m_controller->HandleMessage(message);
	});

	m_bot->getEvents().onCallbackQuery([this](TgBot::CallbackQuery::Ptr query) {
		m_controller->HandleCallbackQuery(query);
	});
}

void ParleyBot::Stop() const
{
	m_workers.Stop();
}

void ParleyBot::PublishCommandMenu() const
{
	auto startCommand = std::make_shared<TgBot::BotCommand>();
	startCommand->command = "start";
	startCommand->description = "Начать";

	m_bot->getApi().setMyCommands({startCommand});
}

void ParleyBot::Run() const
{
	PublishCommandMenu();
	std::cout << "Бот запущен: " << m_bot->getApi().getMe()->username
			  << ", рабочих потоков: " << m_workers.GetThreadCount() << std::endl;

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
