#include "Config.hpp"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string_view>

namespace
{
std::string Trim(const std::string& text)
{
	const auto start = text.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
	{
		return {};
	}
	return text.substr(start, text.find_last_not_of(" \t\r\n") - start + 1);
}

void AssertIsValid(bool condition, const std::string& message)
{
	if (!condition)
	{
		throw std::runtime_error(message);
	}
}

std::string ParseValue(const std::string& raw)
{
	auto value = Trim(raw);
	if (value.empty())
	{
		return value;
	}
	if (value.front() == '"' || value.front() == '\'')
	{
		const auto end = value.find(value.front(), 1);
		AssertIsValid(end != std::string::npos, "Незакрытые кавычки в ENV");
		const auto tail = Trim(value.substr(end + 1));
		AssertIsValid(tail.empty() || tail.starts_with('#'), "Лишние символы после кавычек в ENV");
		return value.substr(1, end - 1);
	}
	const auto comment = value.find(" #");
	return Trim(value.substr(0, comment));
}

std::map<std::string, std::string> ReadEnv(const std::filesystem::path& path)
{
	std::map<std::string, std::string> values;
	if (!std::filesystem::exists(path))
	{
		return values;
	}
	std::ifstream input(path);
	AssertIsValid(input.is_open(), "Не удалось открыть ENV");
	std::string line;
	while (std::getline(input, line))
	{
		if (line.starts_with("\xEF\xBB\xBF"))
		{
			line.erase(0, 3);
		}
		line = Trim(line);
		if (line.empty() || line.starts_with('#'))
		{
			continue;
		}
		if (line.starts_with("export "))
		{
			line.erase(0, 7);
		}
		const auto position = line.find('=');
		AssertIsValid(position != std::string::npos, "В строке ENV отсутствует знак равенства");
		const auto key = Trim(line.substr(0, position));
		AssertIsValid(!key.empty(), "Пустое имя переменной ENV");
		AssertIsValid(!values.contains(key), "Повторное объявление переменной ENV: " + key);
		values.emplace(key, ParseValue(line.substr(position + 1)));
	}
	return values;
}

std::string GetValue(const std::map<std::string, std::string>& values, const std::string& key,
					 const std::string& fallback = {})
{
	if (const auto* value = std::getenv(key.c_str()))
	{
		return Trim(value);
	}
	const auto found = values.find(key);
	return found == values.end() ? fallback : found->second;
}

std::vector<std::int64_t> ParseUsers(const std::string& raw)
{
	AssertIsValid(!raw.empty(), "ALLOWED_USERS не задан: укажите Telegram ID пользователей");
	std::vector<std::int64_t> users;
	std::size_t start = 0;
	do
	{
		const auto end = raw.find(',', start);
		const auto value = Trim(raw.substr(start, end - start));
		std::int64_t userId = 0;
		const auto parsed = std::from_chars(value.data(), value.data() + value.size(), userId);
		AssertIsValid(parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() &&
						  userId > 0,
					  "ALLOWED_USERS должен содержать положительные числа через запятую");
		users.push_back(userId);
		if (end == std::string::npos)
		{
			break;
		}
		start = end + 1;
	} while (start <= raw.size());
	return users;
}
}

Config Config::Load(const std::filesystem::path& envPath)
{
	const auto values = ReadEnv(envPath);
	Config config;
	config.m_token = GetValue(values, "BOT_TOKEN");
	const auto colon = config.m_token.find(':');
	AssertIsValid(colon != std::string::npos && colon > 0 && config.m_token.size() > colon + 10 &&
					  config.m_token.find_first_not_of("0123456789", 0) == colon &&
					  config.m_token.find_first_not_of("0123456789:abcdefghijklmnopqrstuvwxyz"
													   "ABCDEFGHIJKLMNOPQRSTUVWXYZ_- ") ==
						  std::string::npos &&
					  config.m_token.find(' ') == std::string::npos,
				  "BOT_TOKEN не задан или имеет неверный формат");
	config.m_allowedUsers = ParseUsers(GetValue(values, "ALLOWED_USERS"));
	const auto root = std::filesystem::absolute(envPath).parent_path();
	const auto databasePath = GetValue(values, "MATCHES_DB", "data/matches.sqlite3");
	config.m_databasePath =
		std::filesystem::path(std::u8string(databasePath.begin(), databasePath.end()));
	if (config.m_databasePath.is_relative())
	{
		config.m_databasePath = root / config.m_databasePath;
	}
	config.m_seedPath = root / "data/parley_matches_100.sqlite3";
	return config;
}

bool Config::IsAllowed(std::int64_t userId) const noexcept
{
	return std::ranges::find(m_allowedUsers, userId) != m_allowedUsers.end();
}

const std::string& Config::GetToken() const noexcept
{
	return m_token;
}

const std::filesystem::path& Config::GetDatabasePath() const noexcept
{
	return m_databasePath;
}

const std::filesystem::path& Config::GetSeedPath() const noexcept
{
	return m_seedPath;
}
