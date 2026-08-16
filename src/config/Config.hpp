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

private:
	Config(std::string botToken, std::vector<std::string> allowedUsers);

	std::string m_botToken;
	std::vector<std::string> m_allowedUsers;
};