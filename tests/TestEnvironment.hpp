#pragma once

#include "bot/ITelegramClient.hpp"
#include "config/Config.hpp"
#include "storage/MatchStore.hpp"
#include <filesystem>
#include <memory>

struct ApiCall
{
	std::string method;
	nlohmann::json parameters;
};

class FakeTelegramClient : public ITelegramClient
{
  public:
	nlohmann::json Call(const std::string& method, const nlohmann::json& parameters) override;
	std::vector<ApiCall> m_calls;
	int m_failOnSend = 0;
};

class TestEnvironment
{
  public:
	TestEnvironment();
	~TestEnvironment();
	std::filesystem::path m_path;
	Config m_config;
	std::unique_ptr<MatchStore> m_store;
	FakeTelegramClient m_client;
};

nlohmann::json MakeMessage(std::int64_t userId, const std::string& text, std::int64_t updateId = 1);
nlohmann::json MakeCallback(std::int64_t userId, const std::string& data,
							std::int64_t updateId = 2);
