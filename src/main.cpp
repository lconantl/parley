#include "bot/ParleyBot.hpp"
#include "config/Config.hpp"
#include "http/CurlGlobalScope.hpp"
#include "http/TelegramClient.hpp"
#include "storage/MatchStore.hpp"
#include "system/InstanceLock.hpp"
#include "system/StopSignal.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
struct Options
{
	std::filesystem::path envPath = ".env";
	std::filesystem::path databasePath;
	bool check = false;
};

void AssertIsValid(bool condition, const std::string& message)
{
	if (!condition)
	{
		throw std::runtime_error(message);
	}
}

Options ParseOptions(int count, char** values)
{
	Options options;
	for (int index = 1; index < count; ++index)
	{
		const std::string argument = values[index];
		if (argument == "--check")
		{
			options.check = true;
		}
		else
		{
			AssertIsValid((argument == "--env" || argument == "--database") && index + 1 < count,
						  "Использование: parley [--env файл] [--database база] [--check]");
			auto& path = argument == "--env" ? options.envPath : options.databasePath;
			const std::string rawPath = values[++index];
			path = std::filesystem::path(std::u8string(rawPath.begin(), rawPath.end()));
		}
	}
	return options;
}

void PrepareDatabase(const std::filesystem::path& path, const std::filesystem::path& seed)
{
	if (std::filesystem::exists(path))
	{
		return;
	}
	AssertIsValid(std::filesystem::is_regular_file(seed), "Начальная база пар не найдена");
	auto temporary = path;
	temporary += ".tmp";
	std::filesystem::copy_file(seed, temporary, std::filesystem::copy_options::overwrite_existing);
	{
		MatchStore store(temporary);
		store.Validate();
	}
	std::filesystem::rename(temporary, path);
}

int Check(const Options& options)
{
	const auto path = options.databasePath.empty()
						  ? std::filesystem::absolute(options.envPath).parent_path() /
								"data/parley_matches_100.sqlite3"
						  : options.databasePath;
	MatchStore store(path);
	store.Validate();
	std::cout << "База исправна: " << store.GetProgress(0).total
			  << " пар, тексты и связи проверены. Запросов к Telegram не было." << std::endl;
	return 0;
}

int Run(const Options& options)
{
	const auto config = Config::Load(options.envPath);
	const auto path =
		options.databasePath.empty() ? config.GetDatabasePath() : options.databasePath;
	std::filesystem::create_directories(std::filesystem::absolute(path).parent_path());
	const InstanceLock lock(path);
	PrepareDatabase(path, config.GetSeedPath());
	MatchStore store(path);
	store.Validate();
	store.Migrate();
	const CurlGlobalScope httpScope;
	TelegramClient client(config.GetToken());
	ParleyBot bot(config, store, client);
	bot.Run();
	return 0;
}
}

int main(int count, char** values)
{
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif
	StopSignal::Install();
	try
	{
		const auto options = ParseOptions(count, values);
		return options.check ? Check(options) : Run(options);
	}
	catch (const std::exception& error)
	{
		std::cerr << "Не удалось запустить бота: " << error.what() << std::endl;
		return 1;
	}
}
