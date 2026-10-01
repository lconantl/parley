#include "Text.hpp"

#include <stdexcept>

namespace
{
void AssertIsValid(bool condition)
{
	if (!condition)
	{
		throw std::runtime_error("Неверная кодировка текста или длина сообщения");
	}
}

std::size_t CharacterBytes(std::string_view value, std::size_t offset)
{
	const auto first = static_cast<unsigned char>(value[offset]);
	const std::size_t bytes = first < 0x80 ? 1 : first < 0xE0 ? 2 : first < 0xF0 ? 3 : 4;
	AssertIsValid(first < 0x80 || (first >= 0xC2 && first <= 0xF4));
	AssertIsValid(offset + bytes <= value.size());
	for (std::size_t index = 1; index < bytes; ++index)
	{
		AssertIsValid((static_cast<unsigned char>(value[offset + index]) & 0xC0) == 0x80);
	}
	return bytes;
}

std::size_t PrefixBytes(std::string_view value, std::size_t limit)
{
	std::size_t offset = 0;
	std::size_t length = 0;
	while (offset < value.size())
	{
		const auto bytes = CharacterBytes(value, offset);
		const auto units = bytes == 4 ? 2U : 1U;
		if (length + units > limit)
		{
			break;
		}
		length += units;
		offset += bytes;
	}
	return offset;
}
}

std::string Text::Escape(std::string_view value)
{
	std::string result;
	for (const auto character : value)
	{
		switch (character)
		{
		case '&':
			result += "&amp;";
			break;
		case '<':
			result += "&lt;";
			break;
		case '>':
			result += "&gt;";
			break;
		case '"':
			result += "&quot;";
			break;
		default:
			result += character;
			break;
		}
	}
	return result;
}

std::size_t Text::Length(std::string_view value)
{
	std::size_t result = 0;
	for (std::size_t offset = 0; offset < value.size();)
	{
		const auto bytes = CharacterBytes(value, offset);
		result += bytes == 4 ? 2 : 1;
		offset += bytes;
	}
	return result;
}

std::string Text::Shorten(const std::string& value, std::size_t limit)
{
	AssertIsValid(limit >= 2);
	if (Length(value) <= limit)
	{
		return value;
	}
	return value.substr(0, PrefixBytes(value, limit - 1)) + "…";
}

std::vector<std::string> Text::Split(const std::string& value, std::size_t limit)
{
	AssertIsValid(limit >= 2);
	std::vector<std::string> parts;
	std::size_t start = 0;
	while (start < value.size())
	{
		auto count = PrefixBytes(std::string_view(value).substr(start), limit);
		if (start + count < value.size())
		{
			const auto line = value.rfind('\n', start + count - 1);
			if (line != std::string::npos && line >= start + count / 2)
			{
				count = line - start + 1;
			}
		}
		AssertIsValid(count > 0);
		parts.push_back(value.substr(start, count));
		start += count;
	}
	return parts;
}

std::string Text::Date(const std::string& iso)
{
	if (iso.size() < 10)
	{
		return "дата не указана";
	}
	return iso.substr(8, 2) + "." + iso.substr(5, 2) + "." + iso.substr(0, 4);
}
