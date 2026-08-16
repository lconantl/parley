#pragma once

#include <filesystem>
#include <string>
#include <vector>

class Config
{
public:
	static Config Load(const std::filesystem::path& filePath = "../.env");

	[[nodiscard]] const std::string& GetBotToken() const;
	[[nodiscard]] const std::vector<std::string>& GetAllowedUsers() const;

	[[nodiscard]] const std::string& GetDeepSeekApiKey() const;
	[[nodiscard]] const std::string& GetDeepSeekBaseUrl() const;
	[[nodiscard]] const std::string& GetDeepSeekModel() const;

private:
	Config(
		std::string botToken,
		std::vector<std::string> allowedUsers,
		std::string deepSeekApiKey,
		std::string deepSeekBaseUrl,
		std::string deepSeekModel);

	std::string m_botToken;
	std::vector<std::string> m_allowedUsers;
	std::string m_deepSeekApiKey;
	std::string m_deepSeekBaseUrl;
	std::string m_deepSeekModel;
};