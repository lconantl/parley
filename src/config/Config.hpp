#pragma once

#include "EnvLoader.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

class Config
{
public:
	static Config LoadFromEnv(const std::filesystem::path& path = EnvLoader::FILE_NAME);

	const std::string& GetBotToken() const;
	const std::vector<std::int64_t>& GetAllowedUsers() const;
	bool IsUserAllowed(std::int64_t userId) const;
	const std::string& GetPolzaBaseUrl() const;
	const std::string& GetPolzaApiKey() const;
	const std::string& GetPolzaModel() const;
	const std::string& GetCheckoApiKey() const;

private:
	Config() = default;

	std::string m_botToken;
	std::vector<std::int64_t> m_allowedUsers;
	std::string m_polzaBaseUrl;
	std::string m_polzaApiKey;
	std::string m_polzaModel;
	std::string m_checkoApiKey;
};