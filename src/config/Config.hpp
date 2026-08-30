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

	const std::string& GetBotToken() const noexcept
	{
		return m_botToken;
	}

	const std::vector<std::int64_t>& GetAllowedUsers() const noexcept
	{
		return m_allowedUsers;
	}

	bool IsUserAllowed(std::int64_t userId) const;

	const std::string& GetPolzaBaseUrl() const noexcept
	{
		return m_polzaBaseUrl;
	}

	const std::string& GetPolzaApiKey() const noexcept
	{
		return m_polzaApiKey;
	}

	const std::string& GetPolzaModel() const noexcept
	{
		return m_polzaModel;
	}

private:
	Config() = default;

	std::string m_botToken;
	std::vector<std::int64_t> m_allowedUsers;
	std::string m_polzaBaseUrl;
	std::string m_polzaApiKey;
	std::string m_polzaModel;
};