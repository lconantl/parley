#include "ParleyBot.hpp"

#include "bfo/BfoFormatter.hpp"
#include "bfo/BfoMatcher.hpp"
#include "dd/DueDiligenceAnalyzer.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace
{

constexpr std::size_t MAX_PROFILES_PER_QUERY = 5;

constexpr const char* DUE_DILIGENCE_REPORT_TYPES[] = {
	"balance", "financialResult", "capitalChange", "fundsMovement"};

constexpr std::chrono::milliseconds DUE_DILIGENCE_REQUEST_PACING(300);

} // namespace

ParleyBot::ParleyBot(
	const Config& config)
	: m_bot(
		  config.GetBotToken())
	, m_config(
		  config)
	, m_scraper(
		  config)
	, m_polzaClient(
		  config)
{
	SetupHandlers();
}

void ParleyBot::Run() const
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
		throw std::runtime_error(std::string("[Telegram] ") + exception.what());
	}
}

void ParleyBot::SetupHandlers()
{
	m_bot.getEvents().onCommand(
		"start", [this](const TgBot::Message::Ptr& message) {
			if (message == nullptr || !IsUserAllowed(message->from->id))
			{
				return;
			}

			m_bot.getApi().sendMessage(message->chat->id, "Отправь ИНН или название компании.");
		});

	m_bot.getEvents().onCommand(
		"dd",
		[this](
			const TgBot::Message::Ptr& message) {
			if (
				message == nullptr
				|| message->from == nullptr
				|| message->chat == nullptr
				|| !IsUserAllowed(
					message->from->id))
			{
				return;
			}

			const std::string query = ExtractCommandArgument(
				message->text,
				"dd");

			HandleDueDiligenceCommand(
				message->chat->id,
				query);
		});

	m_bot.getEvents().onAnyMessage(
		[this](
			const TgBot::Message::Ptr& message) {
			HandleMessage(
				message);
		});
}

void ParleyBot::HandleMessage(
	const TgBot::Message::Ptr& message) const
{
	if (
		message == nullptr
		|| message->from == nullptr
		|| message->chat == nullptr)
	{
		return;
	}

	if (
		!IsUserAllowed(
			message->from->id))
	{
		return;
	}

	if (message->text.empty())
	{
		return;
	}

	if (
		IsCommand(
			message->text))
	{
		return;
	}

	std::cout
		<< "BFO search: "
		<< message->text
		<< std::endl;

	HandleBfoSearch(
		message->chat->id,
		message->text);
}

bool ParleyBot::IsUserAllowed(const int64_t userId) const
{
	const auto userIdString = std::to_string(userId);
	const auto& users = m_config.GetAllowedUsers();

	return std::ranges::find(users, userIdString) != users.end();
}

void ParleyBot::HandleBfoSearch(const int64_t chatId, const std::string& query) const
{
	try
	{
		m_bot.getApi().sendMessage(chatId, "Ищу компанию на bo.nalog.gov.ru...");

		const SearchResponse response = m_scraper.Search(query);
		const std::vector<CompanySearchResult> matches = BfoMatcher::FilterByQuery(response.companies, query);

		if (matches.empty())
		{
			m_bot.getApi().sendMessage(chatId, "Ничего не найдено с точным совпадением по названию/ИНН.");
			return;
		}

		const std::size_t profileCount = std::min(matches.size(), MAX_PROFILES_PER_QUERY);

		for (std::size_t index = 0; index < profileCount; ++index)
		{
			SendCompanyProfile(chatId, matches[index].id);
		}

		if (matches.size() > profileCount)
		{
			m_bot.getApi().sendMessage(chatId, "Найдено ещё " + std::to_string(matches.size() - profileCount) + " совпадений, уточните запрос.");
		}
	}
	catch (const std::exception& exception)
	{
		std::cerr << "[BFO] " << exception.what() << std::endl;
		m_bot.getApi().sendMessage(chatId, "Не удалось выполнить поиск в БФО. Попробуйте ещё раз позже.");
	}
}

void ParleyBot::SendCompanyProfile(const int64_t chatId, const int organizationId) const
{
	try
	{
		const BfoOrganizationProfile profile = m_scraper.GetOrganizationProfile(organizationId);
		const std::vector<BfoPeriodReport> history = m_scraper.GetOrganizationBfoHistory(organizationId);
		const std::vector<std::string> messages = BfoFormatter::FormatProfile(profile, history);

		for (const auto& message : messages)
		{
			m_bot.getApi().sendMessage(chatId, message, nullptr, nullptr, nullptr, "HTML");
		}
	}
	catch (
		const std::exception& exception)
	{
		std::cerr
			<< "[BFO] Не удалось получить карточку "
			<< organizationId
			<< ": "
			<< exception.what()
			<< std::endl;

		m_bot.getApi().sendMessage(
			chatId,
			"Не удалось получить подробную карточку организации ID "
				+ std::to_string(organizationId)
				+ ".");
	}
}

void ParleyBot::HandleDueDiligenceCommand(
	const int64_t chatId,
	const std::string& query) const
{
	if (query.empty())
	{
		m_bot.getApi().sendMessage(
			chatId,
			"Использование: /dd <ИНН или название компании>");

		return;
	}

	try
	{
		m_bot.getApi().sendMessage(
			chatId,
			"Собираю данные для Due Diligence отчёта, это может занять минуту...");

		const SearchResponse response = m_scraper.Search(
			query);

		const std::vector<CompanySearchResult> matches = BfoMatcher::FilterByQuery(
			response.companies,
			query);

		if (matches.empty())
		{
			m_bot.getApi().sendMessage(
				chatId,
				"Ничего не найдено с точным совпадением по названию/ИНН.");

			return;
		}

		if (matches.size() > 1)
		{
			std::string list = "Найдено несколько совпадений, уточните запрос (например, точным ИНН):\n";

			for (const CompanySearchResult& company : matches)
			{
				list += "- " + company.shortName + " (ИНН " + company.inn + ")\n";
			}

			m_bot.getApi().sendMessage(
				chatId,
				list);

			return;
		}

		const int organizationId = matches.front().id;

		const BfoOrganizationProfile profile = m_scraper.GetOrganizationProfile(
			organizationId);

		const std::vector<BfoPeriodReport> history = m_scraper.GetOrganizationBfoHistory(
			organizationId);

		std::unordered_map<std::string, BfoDetailBreakdown> latestPeriodDetails;

		const BfoPeriodReport* latestPeriod = nullptr;

		for (const BfoPeriodReport& report : history)
		{
			if (
				report.typeCorrections.empty())
			{
				continue;
			}

			if (
				latestPeriod == nullptr
				|| report.period > latestPeriod->period)
			{
				latestPeriod = &report;
			}
		}

		if (
			latestPeriod != nullptr
			&& !latestPeriod->typeCorrections.empty())
		{
			const int correctionId = latestPeriod->typeCorrections.front().correction.id;

			for (const char* reportType : DUE_DILIGENCE_REPORT_TYPES)
			{
				try
				{
					latestPeriodDetails.emplace(
						reportType,
						m_scraper.GetReportDetails(
							correctionId,
							reportType));
				}
				catch (
					const std::exception& detailException)
				{
					std::cerr
						<< "[DD] Не удалось получить детализацию "
						<< reportType
						<< ": "
						<< detailException.what()
						<< std::endl;
				}

				std::this_thread::sleep_for(
					DUE_DILIGENCE_REQUEST_PACING);
			}
		}

		m_bot.getApi().sendMessage(
			chatId,
			"Строю отчёт...");

		const DueDiligenceAnalyzer analyzer(
			profile,
			history,
			latestPeriodDetails,
			m_polzaClient);

		const std::string markdownReport = analyzer.BuildMarkdownReport();

		const std::filesystem::path reportPath = std::filesystem::temp_directory_path()
			/ ("dd_" + profile.inn + ".md");

		{
			std::ofstream file(reportPath, std::ios::binary);
			file << markdownReport;
		}

		m_bot.getApi().sendDocument(
			chatId,
			TgBot::InputFile::fromFile(
				reportPath.string(),
				"text/markdown"),
			"",
			"Due Diligence отчёт: " + profile.shortName);

		std::error_code removeError;
		std::filesystem::remove(reportPath, removeError);
	}
	catch (
		const std::exception& exception)
	{
		std::cerr
			<< "[DD] "
			<< exception.what()
			<< std::endl;

		m_bot.getApi().sendMessage(
			chatId,
			"Не удалось построить Due Diligence отчёт. Попробуйте ещё раз позже.");
	}
}

bool ParleyBot::IsCommand(
	const std::string& text)
{
	return !text.empty()
		&& text.front() == '/';
}

std::string ParleyBot::ExtractCommandArgument(
	const std::string& text,
	const std::string& command)
{
	const std::string prefix = "/" + command;

	if (!text.starts_with(prefix))
	{
		return "";
	}

	std::string rest = text.substr(prefix.size());

	if (
		!rest.empty()
		&& rest.front() == '@')
	{
		const auto spacePosition = rest.find(' ');

		rest = spacePosition == std::string::npos
			? ""
			: rest.substr(spacePosition);
	}

	const auto start = rest.find_first_not_of(" \t");

	return start == std::string::npos
		? ""
		: rest.substr(start);
}