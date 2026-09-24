#include "TestEnvironment.hpp"

#include <atomic>
#include <chrono>
#include <fstream>
#include <stdexcept>

namespace
{
std::filesystem::path CreateDirectory()
{
	static std::atomic<int> sequence{0};
	const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
	const auto path = std::filesystem::temp_directory_path() /
					  ("parley-test-" + std::to_string(now) + "-" + std::to_string(++sequence));
	std::filesystem::create_directories(path);
	return path;
}

Config Prepare(const std::filesystem::path& path)
{
	std::filesystem::copy_file(std::filesystem::path(PARLEY_SOURCE_DIR) /
								   "data/parley_matches_100.sqlite3",
							   path / "matches.sqlite3");
	{
		std::ofstream env(path / ".env");
		env << "BOT_TOKEN=123456:TEST_ONLY_NOT_A_REAL_BOT_TOKEN\n"
			   "ALLOWED_USERS=111, 222\nMATCHES_DB=matches.sqlite3\n";
	}
	return Config::Load(path / ".env");
}
}

nlohmann::json FakeTelegramClient::Call(const std::string& method, const nlohmann::json& parameters)
{
	if (method == "sendMessage" && m_failOnSend > 0 && --m_failOnSend == 0)
	{
		throw std::runtime_error("Проверочная ошибка соединения");
	}
	m_calls.push_back({method, parameters});
	return {{"message_id", static_cast<int>(m_calls.size())}};
}

TestEnvironment::TestEnvironment()
	: m_path(CreateDirectory()), m_config(Prepare(m_path)),
	  m_store(std::make_unique<MatchStore>(m_path / "matches.sqlite3"))
{
	m_store->Validate();
	m_store->Migrate();
}

TestEnvironment::~TestEnvironment()
{
	m_store.reset();
	std::error_code error;
	std::filesystem::remove_all(m_path, error);
}

nlohmann::json MakeMessage(std::int64_t userId, const std::string& text, std::int64_t updateId)
{
	return {{"update_id", updateId},
			{"message",
			 {{"message_id", 10},
			  {"from", {{"id", userId}}},
			  {"chat", {{"id", userId}, {"type", "private"}}},
			  {"text", text}}}};
}

nlohmann::json MakeCallback(std::int64_t userId, const std::string& data, std::int64_t updateId)
{
	const nlohmann::json callback = {{"id", std::to_string(updateId)},
									 {"from", {{"id", userId}}},
									 {"data", data},
									 {"message", MakeMessage(userId, "")["message"]}};
	return {{"update_id", updateId}, {"callback_query", callback}};
}
