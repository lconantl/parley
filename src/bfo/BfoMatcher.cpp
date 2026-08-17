#include "BfoMatcher.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>

namespace
{

constexpr std::array<const char*, 19> LEGAL_FORM_WORDS = {
	"ООО", "ОАО", "ЗАО", "ПАО", "АО", "НАО", "ИП", "НКО", "АНО", "ГУП", "МУП", "ФГУП", "ГКУ", "МКУ", "ОДО", "ТСЖ", "СНТ", "ПК", "СПК"};

bool IsLegalFormWord(
	const std::string& word)
{
	return std::find(
			   LEGAL_FORM_WORDS.begin(),
			   LEGAL_FORM_WORDS.end(),
			   word)
		!= LEGAL_FORM_WORDS.end();
}

std::uint32_t UppercaseCodepoint(
	const std::uint32_t codepoint)
{
	if (codepoint >= 'a' && codepoint <= 'z')
	{
		return codepoint - 'a' + 'A';
	}

	if (codepoint == 0x0451) // ё
	{
		return 0x0401; // Ё
	}

	if (codepoint >= 0x0430 && codepoint <= 0x044F) // а-я
	{
		return codepoint - 0x0430 + 0x0410;
	}

	return codepoint;
}

bool IsLetterOrDigit(
	const std::uint32_t codepoint)
{
	if (codepoint >= '0' && codepoint <= '9')
	{
		return true;
	}

	if (codepoint >= 'A' && codepoint <= 'Z')
	{
		return true;
	}

	if (codepoint == 0x0401) // Ё
	{
		return true;
	}

	if (codepoint >= 0x0410 && codepoint <= 0x042F) // А-Я
	{
		return true;
	}

	return false;
}

void AppendUtf8(
	std::string& out,
	const std::uint32_t codepoint)
{
	if (codepoint < 0x80)
	{
		out += static_cast<char>(codepoint);
	}
	else if (codepoint < 0x800)
	{
		out += static_cast<char>(0xC0 | (codepoint >> 6));
		out += static_cast<char>(0x80 | (codepoint & 0x3F));
	}
	else if (codepoint < 0x10000)
	{
		out += static_cast<char>(0xE0 | (codepoint >> 12));
		out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
		out += static_cast<char>(0x80 | (codepoint & 0x3F));
	}
	else
	{
		out += static_cast<char>(0xF0 | (codepoint >> 18));
		out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
		out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
		out += static_cast<char>(0x80 | (codepoint & 0x3F));
	}
}

std::vector<std::uint32_t> DecodeUtf8(
	const std::string& text)
{
	std::vector<std::uint32_t> result;

	result.reserve(
		text.size());

	std::size_t index = 0;

	while (index < text.size())
	{
		const auto byte0 = static_cast<unsigned char>(text[index]);

		std::uint32_t codepoint = 0;
		std::size_t extraBytes = 0;

		if ((byte0 & 0x80) == 0x00)
		{
			codepoint = byte0;
			extraBytes = 0;
		}
		else if ((byte0 & 0xE0) == 0xC0)
		{
			codepoint = byte0 & 0x1F;
			extraBytes = 1;
		}
		else if ((byte0 & 0xF0) == 0xE0)
		{
			codepoint = byte0 & 0x0F;
			extraBytes = 2;
		}
		else if ((byte0 & 0xF8) == 0xF0)
		{
			codepoint = byte0 & 0x07;
			extraBytes = 3;
		}
		else
		{
			++index;
			continue;
		}

		++index;

		bool valid = true;

		for (std::size_t i = 0; i < extraBytes; ++i)
		{
			if (
				index >= text.size()
				|| (static_cast<unsigned char>(text[index]) & 0xC0) != 0x80)
			{
				valid = false;
				break;
			}

			codepoint = (codepoint << 6) | (static_cast<unsigned char>(text[index]) & 0x3F);
			++index;
		}

		if (valid)
		{
			result.push_back(codepoint);
		}
	}

	return result;
}

} // namespace

std::vector<CompanySearchResult> BfoMatcher::FilterByQuery(
	const std::vector<CompanySearchResult>& companies,
	const std::string& query)
{
	if (IsInnQuery(query))
	{
		std::vector<CompanySearchResult> result;

		for (const auto& company : companies)
		{
			if (company.inn == query)
			{
				result.push_back(company);
			}
		}

		return result;
	}

	const std::string queryCore = NormalizeCore(query);

	if (queryCore.size() < 3)
	{
		return companies;
	}

	std::vector<CompanySearchResult> result;

	for (const auto& company : companies)
	{
		const std::string nameCore = NormalizeCore(company.shortName);

		if (nameCore.find(queryCore) != std::string::npos)
		{
			result.push_back(company);
		}
	}

	return result;
}

std::string BfoMatcher::NormalizeCore(
	const std::string& text)
{
	const std::vector<std::uint32_t> codepoints = DecodeUtf8(text);

	std::vector<std::string> words;
	std::string currentWord;

	for (const std::uint32_t rawCodepoint : codepoints)
	{
		const std::uint32_t codepoint = UppercaseCodepoint(rawCodepoint);

		if (IsLetterOrDigit(codepoint))
		{
			AppendUtf8(currentWord, codepoint);
		}
		else if (!currentWord.empty())
		{
			words.push_back(currentWord);
			currentWord.clear();
		}
	}

	if (!currentWord.empty())
	{
		words.push_back(currentWord);
	}

	std::string core;

	for (const std::string& word : words)
	{
		if (!IsLegalFormWord(word))
		{
			core += word;
		}
	}

	return core;
}

bool BfoMatcher::IsInnQuery(
	const std::string& query)
{
	if (query.empty())
	{
		return false;
	}

	const bool allDigits = std::all_of(
		query.begin(),
		query.end(),
		[](const unsigned char character) {
			return std::isdigit(character) != 0;
		});

	return allDigits
		&& (query.size() == 10 || query.size() == 12);
}
