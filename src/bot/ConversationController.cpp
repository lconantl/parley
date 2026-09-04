#include "ConversationController.hpp"

#include "ArtifactSender.hpp"
#include "KeyboardFactory.hpp"
#include "MessageManager.hpp"
#include "MessageParser.hpp"
#include "PriceLabeler.hpp"
#include "TypingIndicator.hpp"
#include "common/inn/Inn.hpp"
#include "common/pool/WorkerPool.hpp"

#include <atomic>
#include <future>
#include <iostream>
#include <vector>

namespace
{
constexpr auto WelcomeText =
	"Привет, я Parley — бот для анализа российских компаний.\n"
	"\n"
	"Соберу данные из реестров, посчитаю экономические показатели, а пробелы закрою "
	"оценкой по открытым источникам.\n"
	"\n"
	"Выберите, какой материал подготовить:";
constexpr auto AnonymityQuestionText = "Готовить анонимную версию (без названия и ИНН) или обычную?";
constexpr auto InnPromptText = "Пришлите ИНН организации: 10 цифр для юридического лица или 12 для предпринимателя";
constexpr auto InvalidIdentifierText = "Это не похоже на ИНН. Проверьте контрольную сумму и пришлите номер еще раз";
constexpr auto LandingStubText = "Одностраничник пока в разработке, скоро добавим";
constexpr auto StatusText = "Собираю данные и считаю показатели, это займет от 1 до 5 минут";
constexpr auto BusyText = "Предыдущий запрос еще выполняется, дождитесь ответа";
constexpr auto NoDataText = "Данных об этой организации нет ни в реестрах, ни в открытых источниках";
constexpr auto FailureText = "Не удалось выполнить запрос";
constexpr auto MoreComingText = "\n\nГотовится ещё один материал...";

struct DocumentJob
{
	std::shared_ptr<IDocumentBuilder> builder;
	DocumentBuildOptions options;
};

void LogError(const std::exception& error)
{
	std::cerr << "Ошибка бота: " << error.what() << std::endl;
}
} // namespace

ConversationController::ConversationController(
	TgBot::Bot& bot,
	AccessPolicy accessPolicy,
	Dependencies dependencies,
	WorkerPool& workers)
	: m_bot(bot)
	, m_accessPolicy(std::move(accessPolicy))
	, m_dependencies(std::move(dependencies))
	, m_workers(workers)
{
}

void ConversationController::HandleMessage(const TgBot::Message::Ptr& rawMessage)
{
	try
	{
		const ParsedMessage message = MessageParser::Parse(rawMessage);

		if (!m_accessPolicy.IsAllowed(message.userId))
		{
			return;
		}

		if (m_conversations.IsProcessing(message.userId))
		{
			MessageManager messages(&m_bot.getApi(), m_conversations, message.userId);
			messages.SendText(message.chatId, BusyText);
			return;
		}

		const ConversationSession session = m_conversations.Get(message.userId, message.chatId);

		if (session.step == ConversationStep::AwaitingInn)
		{
			OnInnCandidate(message);
			return;
		}

		RestartFlow(message.userId, message.chatId, message.messageId);
	}
	catch (const std::exception& error)
	{
		LogError(error);
	}
}

void ConversationController::HandleCallbackQuery(const TgBot::CallbackQuery::Ptr& query)
{
	try
	{
		if (query == nullptr || query->from == nullptr || query->message == nullptr
			|| query->message->chat == nullptr)
		{
			return;
		}

		const std::int64_t userId = query->from->id;
		const std::int64_t chatId = query->message->chat->id;
		const std::int32_t messageId = query->message->messageId;
		const std::string data = query->data;

		if (!m_accessPolicy.IsAllowed(userId))
		{
			m_bot.getApi().answerCallbackQuery(query->id);
			return;
		}

		if (m_conversations.IsProcessing(userId))
		{
			m_bot.getApi().answerCallbackQuery(query->id, BusyText, true);
			return;
		}

		const ConversationSession session = m_conversations.Get(userId, chatId);
		if (session.promptMessageId != messageId)
		{
			m_bot.getApi().answerCallbackQuery(query->id);
			RestartFlow(userId, chatId);
			return;
		}

		m_bot.getApi().answerCallbackQuery(query->id);

		if (data == "dt:report")
		{
			OnDocumentTypeChosen(userId, chatId, messageId, DocumentSelection::Report);
		}
		else if (data == "dt:pres")
		{
			OnDocumentTypeChosen(userId, chatId, messageId, DocumentSelection::Presentation);
		}
		else if (data == "dt:landing")
		{
			OnDocumentTypeChosen(userId, chatId, messageId, DocumentSelection::OnePager);
		}
		else if (data == "dt:all")
		{
			OnDocumentTypeChosen(userId, chatId, messageId, DocumentSelection::All);
		}
		else if (data == "an:1")
		{
			OnAnonymityChosen(userId, chatId, messageId, true);
		}
		else if (data == "an:0")
		{
			OnAnonymityChosen(userId, chatId, messageId, false);
		}
		else
		{
			RestartFlow(userId, chatId);
		}
	}
	catch (const std::exception& error)
	{
		LogError(error);
	}
}

void ConversationController::RestartFlow(
	const std::int64_t userId,
	const std::int64_t chatId,
	const std::int32_t triggeringMessageId)
{
	MessageManager messages(&m_bot.getApi(), m_conversations, userId);

	if (triggeringMessageId != 0)
	{
		messages.TrackMessage(chatId, triggeringMessageId);
	}

	messages.DeleteTrackedMessages(chatId);

	m_conversations.ResetToIdle(userId, chatId);
	ShowWelcome(userId, chatId);
}

void ConversationController::ShowWelcome(const std::int64_t userId, const std::int64_t chatId)
{
	MessageManager messages(&m_bot.getApi(), m_conversations, userId);

	const PriceQuote prices = PriceLabeler::RollPrices();
	const auto keyboard = KeyboardFactory::DocumentTypeKeyboard(prices);

	const std::int32_t promptId = messages.SendWithKeyboard(chatId, WelcomeText, keyboard);

	m_conversations.SetStep(userId, chatId, ConversationStep::AwaitingDocumentType);
	m_conversations.SetPrompt(userId, chatId, promptId);
}

void ConversationController::OnDocumentTypeChosen(
	const std::int64_t userId,
	const std::int64_t chatId,
	const std::int32_t promptMessageId,
	const DocumentSelection choice)
{
	MessageManager messages(&m_bot.getApi(), m_conversations, userId);
	m_conversations.SetSelection(userId, chatId, choice);

	if (choice == DocumentSelection::OnePager)
	{
		messages.EditText(chatId, promptMessageId, LandingStubText, KeyboardFactory::Empty());
		m_conversations.ResetToIdle(userId, chatId);
		return;
	}

	if (choice == DocumentSelection::Report)
	{
		messages.EditText(chatId, promptMessageId, InnPromptText, KeyboardFactory::Empty());
		m_conversations.SetStep(userId, chatId, ConversationStep::AwaitingInn);
		return;
	}

	messages.EditText(chatId, promptMessageId, AnonymityQuestionText, KeyboardFactory::AnonymityKeyboard());
	m_conversations.SetStep(userId, chatId, ConversationStep::AwaitingAnonymity);
}

void ConversationController::OnAnonymityChosen(
	const std::int64_t userId,
	const std::int64_t chatId,
	const std::int32_t promptMessageId,
	const bool anonymize)
{
	MessageManager messages(&m_bot.getApi(), m_conversations, userId);

	m_conversations.SetAnonymize(userId, chatId, anonymize);
	messages.EditText(chatId, promptMessageId, InnPromptText, KeyboardFactory::Empty());
	m_conversations.SetStep(userId, chatId, ConversationStep::AwaitingInn);
}

void ConversationController::OnInnCandidate(const ParsedMessage& message)
{
	MessageManager messages(&m_bot.getApi(), m_conversations, message.userId);
	messages.TrackMessage(message.chatId, message.messageId);

	const std::string identifier = Inn::Extract(message.text);
	if (!identifier.empty())
	{
		BeginProcessing(message.userId, message.chatId, identifier);
		return;
	}

	const bool hasDigits = !Inn::Normalize(message.text).empty();
	messages.SendText(message.chatId, hasDigits ? InvalidIdentifierText : InnPromptText);
}

void ConversationController::BeginProcessing(
	const std::int64_t userId,
	const std::int64_t chatId,
	const std::string& inn)
{
	if (!m_conversations.TryEnterProcessing(userId, chatId))
	{
		return;
	}

	MessageManager messages(&m_bot.getApi(), m_conversations, userId);
	messages.SendText(chatId, StatusText);

	m_workers.Post([this, userId, chatId, inn] { RunAnalysisJob(userId, chatId, inn); });
}

void ConversationController::RunAnalysisJob(
	const std::int64_t userId,
	const std::int64_t chatId,
	std::string inn)
{
	const TypingIndicator typing(&m_bot.getApi(), chatId);

	CompanyAnalytics analytics;
	try
	{
		analytics = m_dependencies.analyticsViewModel->Analyze(inn);
	}
	catch (const std::exception& error)
	{
		MessageManager messages(&m_bot.getApi(), m_conversations, userId);
		messages.SendText(chatId, std::string(FailureText) + ": " + error.what());
		m_conversations.ResetToIdle(userId, chatId);
		return;
	}

	if (analytics.status == AnalyticsStatus::NoData)
	{
		MessageManager messages(&m_bot.getApi(), m_conversations, userId);
		messages.SendText(chatId, NoDataText);
		m_conversations.ResetToIdle(userId, chatId);
		return;
	}

	const ConversationSession session = m_conversations.Get(userId, chatId);

	DocumentBuildOptions reportOptions;
	reportOptions.anonymize = false;
	reportOptions.showSourceNotes = m_dependencies.showSourceNotes;
	reportOptions.author = m_dependencies.author;

	DocumentBuildOptions presentationOptions;
	presentationOptions.anonymize = session.anonymize;
	presentationOptions.showSourceNotes = m_dependencies.showSourceNotes;
	presentationOptions.author = m_dependencies.author;

	std::vector<DocumentJob> jobs;
	if (session.selection == DocumentSelection::Report || session.selection == DocumentSelection::All)
	{
		jobs.push_back({m_dependencies.reportBuilder, reportOptions});
	}
	if (session.selection == DocumentSelection::Presentation || session.selection == DocumentSelection::All)
	{
		jobs.push_back({m_dependencies.presentationBuilder, presentationOptions});
	}

	if (jobs.empty())
	{
		m_conversations.ResetToIdle(userId, chatId);
		return;
	}

	std::atomic<int> remaining{static_cast<int>(jobs.size())};
	std::atomic<bool> trailWiped{false};

	auto runJob = [&](const DocumentJob& job) {
		DocumentBuildResult result;
		bool succeeded = false;

		try
		{
			result = job.builder->Build(analytics, job.options);
			succeeded = true;
		}
		catch (const std::exception& error)
		{
			std::cerr << "document build failed (" << job.builder->GetLabel() << "): "
					  << error.what() << std::endl;
		}

		const int othersPending = remaining.fetch_sub(1) - 1;

		if (!succeeded)
		{
			MessageManager messages(&m_bot.getApi(), m_conversations, userId);
			messages.SendText(chatId, job.builder->GetLabel() + ": не удалось подготовить документ");
			return;
		}

		if (othersPending > 0)
		{
			result.caption += MoreComingText;
		}

		if (!trailWiped.exchange(true))
		{
			MessageManager messages(&m_bot.getApi(), m_conversations, userId);
			messages.DeleteTrackedMessages(chatId);
		}

		ArtifactSender(&m_bot.getApi()).SendDocument(chatId, result.path, result.caption, result.mimeType);
	};

	std::vector<std::future<void>> tasks;
	tasks.reserve(jobs.size());

	for (const auto& job : jobs)
	{
		tasks.push_back(std::async(std::launch::async, runJob, job));
	}

	for (auto& task : tasks)
	{
		task.get();
	}

	m_conversations.ResetToIdle(userId, chatId);
}
