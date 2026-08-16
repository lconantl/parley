#include "Config.hpp"
#include "EnvLoader.hpp"
#include <stdexcept>

namespace
{
const std::string BOT_TOKEN_KEY = "BOT_TOKEN";
const std::string ALLOWED_USERS_KEY = "ALLOWED_USERS";

void AssertIsKeyFound(const bool isFound)
{
	if (!isFound)
	{
		throw std::runtime_error("В файле конфигурации отсутствует обязательный ключ");
	}
}

void AssertIsValidToken(const std::string& token)
{
	if (token.find(':') == std::string::npos)
	{
		throw std::runtime_error("Токен бота имеет неверный формат");
	}
}

std::string ExtractRequiredValue(const EnvLoader::EnvData& data, const std::string& key)
{
	const auto iterator = data.find(key);
	AssertIsKeyFound(iterator != data.end());
	return iterator->second;
}

std::vector<std::string> ParseUsersList(const std::string& rawList)
{
	std::vector<std::string> users;
	size_t start = 0;
	size_t end = rawList.find(',');

	while (end != std::string::npos)
	{
		if (start != end)
		{
			users.push_back(rawList.substr(start, end - start));
		}
		start = end + 1;
		end = rawList.find(',', start);
	}

	if (start < rawList.size())
	{
		users.push_back(rawList.substr(start));
	}

	return users;
}
} // namespace

Config Config::Load(const std::filesystem::path& filePath)
{
	const auto envData = EnvLoader::Load(filePath);
	const auto botToken = ExtractRequiredValue(envData, BOT_TOKEN_KEY);
	const auto rawAllowedUsers = ExtractRequiredValue(envData, ALLOWED_USERS_KEY);

	AssertIsValidToken(botToken);
	const auto allowedUsers = ParseUsersList(rawAllowedUsers);

	return {botToken, allowedUsers};
}

Config::Config(std::string botToken, std::vector<std::string> allowedUsers)
	: m_botToken(std::move(botToken))
	, m_allowedUsers(std::move(allowedUsers))
{
}

const std::string& Config::GetBotToken() const
{
	return m_botToken;
}

const std::vector<std::string>& Config::GetAllowedUsers() const
{
	return m_allowedUsers;
}