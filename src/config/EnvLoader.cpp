#include "EnvLoader.hpp"

#include <fstream>
#include <iostream>

namespace
{
constexpr std::string_view TAG = "[EnvLoader]\t";
constexpr std::string_view WHITESPACE = " \t\r\n\v\f";
constexpr std::string_view EXPORT_PREFIX = "export ";
constexpr char DELIMITER = '=';
constexpr char COMMENT = '#';
constexpr char QUOTE = '"';
constexpr char ESCAPE = '\\';

void LogWarning(const std::string& message)
{
	std::clog << TAG << message << '\n';
}

EnvLoader::LoadError MakeError(const std::string& message)
{
	return EnvLoader::LoadError(std::string(TAG) + message);
}

EnvLoader::LoadError MakeError(const size_t lineNumber, const std::string& message)
{
	return MakeError("(line-" + std::to_string(lineNumber) + "): " + message);
}

bool IsWhitespace(const char ch)
{
	return WHITESPACE.find(ch) != std::string_view::npos;
}

bool IsUpperLetter(const char ch)
{
	return ch >= 'A' && ch <= 'Z';
}

bool IsLowerLetter(const char ch)
{
	return ch >= 'a' && ch <= 'z';
}

bool IsDigit(const char ch)
{
	return ch >= '0' && ch <= '9';
}

bool IsBlankOrComment(const std::string& line)
{
	const auto pos = line.find_first_not_of(WHITESPACE);
	return pos == std::string::npos || line[pos] == COMMENT;
}

std::string DropExportPrefix(const std::string& line, const size_t lineNumber)
{
	const auto start = line.find_first_not_of(WHITESPACE);
	if (line.compare(start, EXPORT_PREFIX.size(), EXPORT_PREFIX) != 0)
	{
		return line;
	}

	LogWarning("Строка " + std::to_string(lineNumber) + ": префикс 'export' не поддерживается и проигнорирован");
	return line.substr(start + EXPORT_PREFIX.size());
}

std::string RemoveWhitespaceOutsideQuotes(const std::string& line, const size_t lineNumber)
{
	std::string result;
	result.reserve(line.size());

	bool isQuoted = false;
	bool isEscaped = false;

	for (const char ch : line)
	{
		if (isEscaped)
		{
			isEscaped = false;
		}
		else if (isQuoted && ch == ESCAPE)
		{
			isEscaped = true;
		}
		else if (ch == QUOTE)
		{
			isQuoted = !isQuoted;
		}
		else if (!isQuoted && IsWhitespace(ch))
		{
			continue;
		}

		result += ch;
	}

	if (isQuoted)
	{
		throw MakeError(lineNumber, "Незакрытая кавычка: " + line);
	}

	return result;
}

void AssertIsValidKey(const std::string& key, const size_t lineNumber)
{
	if (key.empty())
	{
		throw MakeError(lineNumber, "Пустое имя переменной");
	}

	for (const char ch : key)
	{
		if (IsLowerLetter(ch))
		{
			throw MakeError(lineNumber, "Имя переменной должно быть в верхнем регистре: " + key);
		}

		if (!IsUpperLetter(ch) && !IsDigit(ch) && ch != '_')
		{
			throw MakeError(lineNumber, "Имя переменной допускает только буквы, цифры и '_': " + key);
		}
	}

	if (!IsUpperLetter(key.front()))
	{
		throw MakeError(lineNumber, "Имя переменной должно начинаться с буквы: " + key);
	}

	if (key.back() == '_')
	{
		throw MakeError(lineNumber, "Имя переменной не может заканчиваться на '_': " + key);
	}
}

size_t FindClosingQuote(const std::string& value)
{
	bool isEscaped = false;

	for (size_t i = 1; i < value.size(); ++i)
	{
		if (isEscaped)
		{
			isEscaped = false;
		}
		else if (value[i] == ESCAPE)
		{
			isEscaped = true;
		}
		else if (value[i] == QUOTE)
		{
			return i;
		}
	}

	return std::string::npos;
}

std::string Unescape(const std::string& quotedBody)
{
	std::string result;
	result.reserve(quotedBody.size());

	for (size_t i = 0; i < quotedBody.size(); ++i)
	{
		const bool isEscapePair = quotedBody[i] == ESCAPE
			&& i + 1 < quotedBody.size()
			&& (quotedBody[i + 1] == QUOTE || quotedBody[i + 1] == ESCAPE);

		if (isEscapePair)
		{
			++i;
		}

		result += quotedBody[i];
	}

	return result;
}

void AssertIsPlainValue(const std::string& value, const size_t lineNumber)
{
	if (value.find(QUOTE) != std::string::npos)
	{
		throw MakeError(lineNumber, "Кавычки должны обрамлять значение целиком: " + value);
	}

	for (const char ch : value)
	{
		if (ch == DELIMITER || ch == COMMENT)
		{
			throw MakeError(lineNumber, std::string("Символ '") + ch + "' в значении требует кавычек, пишите KEY=\"" + value + "\"");
		}
	}
}

std::string ExtractValue(const std::string& rawValue, const size_t lineNumber)
{
	if (rawValue.empty() || rawValue.front() != QUOTE)
	{
		AssertIsPlainValue(rawValue, lineNumber);
		return rawValue;
	}

	const auto closingPos = FindClosingQuote(rawValue);
	if (closingPos == std::string::npos)
	{
		throw MakeError(lineNumber, "Незакрытая кавычка: " + rawValue);
	}

	if (closingPos + 1 != rawValue.size())
	{
		throw MakeError(lineNumber, "Лишние символы после закрывающей кавычки: " + rawValue);
	}

	return Unescape(rawValue.substr(1, closingPos - 1));
}

void AssertIsFileExist(const std::filesystem::path& path)
{
	if (path.empty())
	{
		throw MakeError("Путь к файлу окружения пуст");
	}
}

void AssertIsFileOpen(const std::ifstream& file, const std::filesystem::path& path)
{
	if (!file.is_open())
	{
		throw MakeError("Не удалось открыть файл окружения: " + path.string());
	}
}

void AssertIsDelimiterExist(const size_t delimiterPos, const size_t lineNumber)
{
	if (delimiterPos == std::string::npos)
	{
		throw MakeError(lineNumber, "Нет разделителя '='");
	}
}

void AssertIsValueExist(const std::string& value, const std::string& key)
{
	if (value.empty())
	{
		LogWarning("Значение для переменной " + key + " не задано");
	}
}

void AssertIsNotRepetitions(EnvLoader::EnvData& data, const std::string& key, const std::string& value, const size_t lineNumber)
{
	if (!data.emplace(key, value).second)
	{
		throw MakeError(lineNumber, "Повторное объявление переменной: " + key);
	}
}

void WarnIfMockFile(const std::filesystem::path& path)
{
	if (path.filename() == EnvLoader::MOCK_FILE_NAME)
	{
		LogWarning("Используется моковый файл окружения: " + path.string());
	}
}
} // namespace

EnvLoader::EnvData EnvLoader::Load(const std::filesystem::path& path)
{
	AssertIsFileExist(path);
	std::ifstream file(path);

	AssertIsFileOpen(file, path);
	WarnIfMockFile(path);

	EnvData data;
	std::string line;
	size_t lineNumber = 0;

	while (std::getline(file, line))
	{
		++lineNumber;

		if (IsBlankOrComment(line))
		{
			continue;
		}

		const auto compactLine = RemoveWhitespaceOutsideQuotes(DropExportPrefix(line, lineNumber), lineNumber);

		const auto delimiterPos = compactLine.find(DELIMITER);
		AssertIsDelimiterExist(delimiterPos, lineNumber);

		const auto key = compactLine.substr(0, delimiterPos);
		AssertIsValidKey(key, lineNumber);

		const auto value = ExtractValue(compactLine.substr(delimiterPos + 1), lineNumber);
		AssertIsValueExist(value, key);

		AssertIsNotRepetitions(data, key, value, lineNumber);
	}

	return data;
}

void EnvLoader::PrintMap(const EnvData& data)
{
	for (const auto& [key, value] : data)
	{
		std::cout << key << " = " << value << '\n';
	}
}