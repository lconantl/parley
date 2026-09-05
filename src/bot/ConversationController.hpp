#pragma once

#include "AccessPolicy.hpp"
#include "ConversationRegistry.hpp"
#include "ParsedMessage.hpp"
#include "documents/IDocumentBuilder.hpp"
#include "viewmodel/CompanyAnalyticsViewModel.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <tgbot/tgbot.h>

class WorkerPool;

// Ведёт весь диалог с пользователем: приветствие с кнопками -> выбор типа материала ->
// (опционально) анонимность -> запрос ИНН -> фоновая обработка -> выдача документов.
// Заменяет собой прежние CommandRouter/ICommandHandler — единственная точка входа для
// onAnyMessage и onCallbackQuery.
class ConversationController
{
public:
	struct Dependencies
	{
		std::shared_ptr<CompanyAnalyticsViewModel> analyticsViewModel;
		std::shared_ptr<IDocumentBuilder> reportBuilder;
		std::shared_ptr<IDocumentBuilder> presentationBuilder;
		std::shared_ptr<IDocumentBuilder> onePagerBuilder;
		bool showSourceNotes = false;
		std::string author = "Investment Analysis";
	};

	ConversationController(
		TgBot::Bot& bot,
		AccessPolicy accessPolicy,
		Dependencies dependencies,
		WorkerPool& workers);

	void HandleMessage(const TgBot::Message::Ptr& rawMessage);
	void HandleCallbackQuery(const TgBot::CallbackQuery::Ptr& query);

private:
	void RestartFlow(std::int64_t userId, std::int64_t chatId, std::int32_t triggeringMessageId = 0);
	void ShowWelcome(std::int64_t userId, std::int64_t chatId);

	void OnDocumentTypeChosen(
		std::int64_t userId,
		std::int64_t chatId,
		std::int32_t promptMessageId,
		DocumentSelection choice);
	void OnAnonymityChosen(
		std::int64_t userId,
		std::int64_t chatId,
		std::int32_t promptMessageId,
		bool anonymize);
	void OnInnCandidate(const ParsedMessage& message);

	void BeginProcessing(std::int64_t userId, std::int64_t chatId, const std::string& inn);
	void RunAnalysisJob(std::int64_t userId, std::int64_t chatId, std::string inn);

	TgBot::Bot& m_bot;
	AccessPolicy m_accessPolicy;
	Dependencies m_dependencies;
	WorkerPool& m_workers;
	ConversationRegistry m_conversations;
};
