#include "ContactNormalizer.hpp"

#include <algorithm>
#include <cctype>

namespace
{
bool IsDigitChar(const char ch)
{
	return ch >= '0' && ch <= '9';
}

std::string OnlyDigits(const std::string& value)
{
	std::string result;
	for (const char ch : value)
	{
		if (IsDigitChar(ch))
		{
			result += ch;
		}
	}

	return result;
}

std::string ToLowerCopy(const std::string& value)
{
	std::string result = value;
	std::ranges::transform(result, result.begin(), [](const unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});

	return result;
}

std::string TrimCopy(const std::string& value)
{
	const auto begin = value.find_first_not_of(" \t\r\n");
	if (begin == std::string::npos)
	{
		return "";
	}

	const auto end = value.find_last_not_of(" \t\r\n");
	return value.substr(begin, end - begin + 1);
}
} // namespace

std::string ContactNormalizer::NormalizePhone(const std::string& rawPhone)
{
	std::string digits = OnlyDigits(rawPhone);
	if (digits.empty())
	{
		return "";
	}

	if (digits.size() > 11 && digits.starts_with("00"))
	{
		digits = digits.substr(2);
	}

	if (digits.size() == 11 && (digits.front() == '8' || digits.front() == '7'))
	{
		digits = "7" + digits.substr(1);
	}
	else if (digits.size() == 10)
	{
		digits = "7" + digits;
	}

	return "+" + digits;
}

std::string ContactNormalizer::NormalizeTelegramUsername(const std::string& rawUsername)
{
	std::string trimmed = TrimCopy(rawUsername);
	if (!trimmed.empty() && trimmed.front() == '@')
	{
		trimmed = trimmed.substr(1);
	}

	return ToLowerCopy(trimmed);
}

std::string ContactNormalizer::BuildCanonicalName(const std::string& firstName, const std::string& lastName)
{
	const std::string first = TrimCopy(firstName);
	const std::string last = TrimCopy(lastName);

	if (first.empty())
	{
		return last;
	}

	if (last.empty())
	{
		return first;
	}

	return last + " " + first;
}

Contact ContactNormalizer::Normalize(const RawContact& raw)
{
	Contact contact;
	contact.canonicalName = BuildCanonicalName(raw.firstName, raw.lastName);
	contact.role = TrimCopy(raw.role);
	contact.location = TrimCopy(raw.location);
	contact.notes = TrimCopy(raw.notes);
	contact.sources = {raw.source};

	for (const auto& channel : raw.channels)
	{
		ContactChannel normalized = channel;

		if (channel.type == ChannelType::Phone)
		{
			normalized.value = NormalizePhone(channel.value);
		}
		else if (channel.type == ChannelType::TelegramUsername)
		{
			normalized.value = NormalizeTelegramUsername(channel.value);
		}
		else
		{
			normalized.value = TrimCopy(channel.value);
		}

		if (!normalized.value.empty())
		{
			contact.channels.push_back(normalized);
		}
	}

	return contact;
}
