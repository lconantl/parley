#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

struct BotConfig
{
	std::string token;
	std::vector<std::string> allowedUsers;
};

struct PolzaConfig
{
	std::string baseUrl;
	std::string apiKey;
	std::string model;
};

struct Config
{
	BotConfig bot;
	PolzaConfig polza;
};

Config Load(const std::filesystem::path& path);