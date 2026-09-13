#include "application/importer/ContactImportService/ContactImportService.hpp"
#include "application/search/ContactSearchService/ContactSearchService.hpp"
#include "bot/ContactSearchBot/ContactSearchBot.hpp"
#include "bot/access/AccessPolicy.hpp"
#include "console/ConsoleEncoding.hpp"
#include "infrastructure/ai/PolzaQueryInterpreter/PolzaQueryInterpreter.hpp"
#include "infrastructure/config/Config.hpp"
#include "infrastructure/import/CsvContactsSource/CsvContactsSource.hpp"
#include "infrastructure/import/ExcelContactsSource/ExcelContactsSource.hpp"
#include "infrastructure/import/TelegramContactsSource/TelegramContactsSource.hpp"
#include "infrastructure/import/VCardContactsSource/VCardContactsSource.hpp"
#include "infrastructure/storage/SqliteContactRepository/SqliteContactRepository.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
constexpr auto ImportCommand = "import";

std::unique_ptr<IContactSource> CreateSource(const std::string& sourceType)
{
	if (sourceType == "telegram")
	{
		return std::make_unique<TelegramContactsSource>();
	}
	if (sourceType == "vcard")
	{
		return std::make_unique<VCardContactsSource>();
	}
	if (sourceType == "excel")
	{
		return std::make_unique<ExcelContactsSource>();
	}
	if (sourceType == "csv")
	{
		return std::make_unique<CsvContactsSource>();
	}

	throw std::invalid_argument(
		"Неизвестный тип источника: " + sourceType + " (ожидается telegram, vcard, excel или csv)");
}

void PrintImportUsage()
{
	std::cerr << "Использование: parley import <telegram|vcard|excel|csv> <путь-к-файлу>" << std::endl;
}

int RunImport(const Config& config, const int argc, char** argv)
{
	if (argc < 4)
	{
		PrintImportUsage();
		return EXIT_FAILURE;
	}

	const std::string sourceType = argv[2];
	const std::filesystem::path sourcePath = argv[3];

	const auto repository = std::make_shared<SqliteContactRepository>(config.GetContactsDbPath());
	const ContactImportService importService(repository);
	const auto source = CreateSource(sourceType);

	const std::size_t imported = importService.ImportFrom(*source, sourcePath);
	std::cout << "Импортировано контактов: " << imported << std::endl;

	return EXIT_SUCCESS;
}

int RunBot(const Config& config)
{
	const auto repository = std::make_shared<SqliteContactRepository>(config.GetContactsDbPath());
	const auto interpreter = std::make_shared<PolzaQueryInterpreter>(
		config.GetPolzaBaseUrl(), config.GetPolzaApiKey(), config.GetPolzaModel());
	const auto searchService = std::make_shared<ContactSearchService>(repository, interpreter);

	const ContactSearchBot bot(config.GetBotToken(), AccessPolicy(config.GetAllowedUsers()), searchService);
	bot.Run();

	return EXIT_SUCCESS;
}
} // namespace

int main(const int argc, char** argv)
{
	ConsoleEncoding encoding;

	try
	{
		std::cout << "Parley contacts search bot" << std::endl;

		const Config config = Config::LoadFromEnv();

		if (argc >= 2 && std::string(argv[1]) == ImportCommand)
		{
			return RunImport(config, argc, argv);
		}

		return RunBot(config);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "[Error] " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}
}
