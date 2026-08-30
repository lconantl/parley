#include "Config.hpp"

#include <algorithm>
#include <charconv>
#include <stdexcept>

namespace
{
constexpr std::string_view TAG = "[Config]\t\t";

constexpr std::string_view BOT_TOKEN = "BOT_TOKEN";
constexpr std::string_view ALLOWED_USERS = "ALLOWED_USERS";
constexpr std::string_view POLZA_BASE_URL = "POLZA_BASE_URL";
constexpr std::string_view POLZA_API_KEY = "POLZA_API_KEY";
constexpr std::string_view POLZA_MODEL = "POLZA_MODEL";

constexpr char LIST_SEPARATOR = ',';

std::runtime_error MakeError(const std::string_view key, const std::string& message)
{
	return std::runtime_error(std::string(TAG) + std::string(key) + ": " + message);
}

std::string RequireString(const EnvLoader::EnvData& data, const std::string_view key)
{
	const auto it = data.find(std::string(key));
	if (it == data.end())
	{
		throw MakeError(key, "переменная не задана");
	}

	if (it->second.empty())
	{
		throw MakeError(key, "значение пустое");
	}

	return it->second;
}

std::string RequireUrl(const EnvLoader::EnvData& data, const std::string_view key)
{
	std::string url = RequireString(data, key);
	if (!url.starts_with("http://") && !url.starts_with("https://"))
	{
		throw MakeError(key, "ожидается http:// или https://, получено '" + url + "'");
	}

	while (url.ends_with('/'))
	{
		url.pop_back();
	}

	return url;
}

std::int64_t ToNumber(const std::string_view key, const std::string& item)
{
	std::int64_t number = 0;
	const char* const begin = item.data();
	const char* const end = item.data() + item.size();

	const auto [stopped, error] = std::from_chars(begin, end, number);
	if (error != std::errc{} || stopped != end || number <= 0)
	{
		throw MakeError(key, "ожидается положительное число, получено '" + item + "'");
	}

	return number;
}

std::vector<std::int64_t> RequireNumberList(const EnvLoader::EnvData& data, const std::string_view key)
{
	const auto raw = RequireString(data, key);

	std::vector<std::int64_t> result;
	size_t start = 0;

	while (true)
	{
		const auto separatorPos = raw.find(LIST_SEPARATOR, start);
		const auto end = separatorPos == std::string::npos ? raw.size() : separatorPos;

		const auto item = raw.substr(start, end - start);
		if (item.empty())
		{
			throw MakeError(key, "пустой элемент списка в '" + raw + "'");
		}

		result.push_back(ToNumber(key, item));

		if (separatorPos == std::string::npos)
		{
			return result;
		}

		start = separatorPos + 1;
	}
}
} // namespace

Config Config::LoadFromEnv(const std::filesystem::path& path)
{
	const EnvLoader::EnvData data = EnvLoader::Load(path);

	Config config;
	config.m_botToken = RequireString(data, BOT_TOKEN);
	config.m_allowedUsers = RequireNumberList(data, ALLOWED_USERS);
	config.m_polzaBaseUrl = RequireUrl(data, POLZA_BASE_URL);
	config.m_polzaApiKey = RequireString(data, POLZA_API_KEY);
	config.m_polzaModel = RequireString(data, POLZA_MODEL);

	return config;
}

bool Config::IsUserAllowed(const std::int64_t userId) const
{
	return std::ranges::find(m_allowedUsers, userId) != m_allowedUsers.end();
}