#include "EnvLoader.hpp"

#include <fstream>

namespace
{
void AssertIsFileOpen(const std::ifstream& file)
{
	if (!file.is_open())
	{
		throw std::runtime_error("Не удалось открыть файл окружения по указанному пути");
	}
}

bool IsEmptyOrComment(const std::string& line)
{
	return line.empty() || line.starts_with('#');
}

bool HasDelimiter(const std::string& line)
{
	return line.find('=') != std::string::npos;
}

std::string Trim(const std::string& str)
{
	const auto start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
	{
		return "";
	}

	const auto end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

std::string ExtractKey(const std::string& line)
{
	const auto delimiterPos = line.find('=');
	return Trim(line.substr(0, delimiterPos));
}

std::string ExtractValue(const std::string& line)
{
	const auto delimiterPos = line.find('=');
	return Trim(line.substr(delimiterPos + 1));
}
} // namespace

EnvLoader::EnvData EnvLoader::Load(const std::filesystem::path& filePath)
{
	EnvData data;
	std::ifstream file(filePath);

	AssertIsFileOpen(file);

	std::string line;
	while (std::getline(file, line))
	{
		std::string trimmedLine = Trim(line);

		if (IsEmptyOrComment(trimmedLine))
		{
			continue;
		}

		if (HasDelimiter(trimmedLine))
		{
			const auto key = ExtractKey(trimmedLine);
			const auto value = ExtractValue(trimmedLine);
			data.emplace(key, value);
		}
	}

	return data;
}