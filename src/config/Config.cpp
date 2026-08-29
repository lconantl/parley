#include "Config.hpp"
#include "EnvLoader.hpp"
#include <stdexcept>
#include <utility>

namespace
{
std::string GetRequired(const EnvLoader::EnvData& data, const std::string& key)
{
	const auto iterator = data.find(key);

	if (iterator == data.end())
	{
		throw std::runtime_error("Отсутствует переменная окружения: " + key);
	}

	return iterator->second;
}

long GetLong(const EnvLoader::EnvData& data, const std::string& key)
{
	try
	{
		return std::stol(GetRequired(data, key));
	}
	catch (...)
	{
		throw std::runtime_error("Некорректное числовое значение: " + key);
	}
}

std::vector<std::string> ParseUsers(const std::string& raw)
{
	std::vector<std::string> result;
	std::size_t start = 0;

	while (start < raw.size())
	{
		const auto comma = raw.find(',', start);

		const auto length = comma == std::string::npos
			? raw.size() - start
			: comma - start;

		if (length > 0)
		{
			result.emplace_back(
				raw.substr(
					start,
					length));
		}

		if (comma == std::string::npos)
		{
			break;
		}

		start = comma + 1;
	}

	return result;
}

} // namespace

Config Load(const std::filesystem::path& path)
{
	const auto data = EnvLoader::Load(path);
	EnvLoader::PrintMap(data);

	return Config();
}