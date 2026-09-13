#include "ContactRowMapper.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace
{
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

std::vector<char32_t> DecodeUtf8(const std::string& text)
{
	std::vector<char32_t> codepoints;
	std::size_t i = 0;

	while (i < text.size())
	{
		const auto byte0 = static_cast<unsigned char>(text[i]);
		char32_t codepoint = byte0;
		std::size_t length = 1;

		if ((byte0 & 0xE0) == 0xC0 && i + 1 < text.size())
		{
			codepoint = static_cast<char32_t>((byte0 & 0x1Fu) << 6 | (static_cast<unsigned char>(text[i + 1]) & 0x3Fu));
			length = 2;
		}
		else if ((byte0 & 0xF0) == 0xE0 && i + 2 < text.size())
		{
			codepoint = static_cast<char32_t>(
				(byte0 & 0x0Fu) << 12 | (static_cast<unsigned char>(text[i + 1]) & 0x3Fu) << 6
				| (static_cast<unsigned char>(text[i + 2]) & 0x3Fu));
			length = 3;
		}
		else if ((byte0 & 0xF8) == 0xF0 && i + 3 < text.size())
		{
			codepoint = static_cast<char32_t>(
				(byte0 & 0x07u) << 18 | (static_cast<unsigned char>(text[i + 1]) & 0x3Fu) << 12
				| (static_cast<unsigned char>(text[i + 2]) & 0x3Fu) << 6
				| (static_cast<unsigned char>(text[i + 3]) & 0x3Fu));
			length = 4;
		}

		codepoints.push_back(codepoint);
		i += length;
	}

	return codepoints;
}

std::string EncodeUtf8(const std::vector<char32_t>& codepoints)
{
	std::string result;

	for (const char32_t codepoint : codepoints)
	{
		if (codepoint < 0x80)
		{
			result += static_cast<char>(codepoint);
		}
		else if (codepoint < 0x800)
		{
			result += static_cast<char>(0xC0u | (codepoint >> 6));
			result += static_cast<char>(0x80u | (codepoint & 0x3Fu));
		}
		else if (codepoint < 0x10000)
		{
			result += static_cast<char>(0xE0u | (codepoint >> 12));
			result += static_cast<char>(0x80u | ((codepoint >> 6) & 0x3Fu));
			result += static_cast<char>(0x80u | (codepoint & 0x3Fu));
		}
		else
		{
			result += static_cast<char>(0xF0u | (codepoint >> 18));
			result += static_cast<char>(0x80u | ((codepoint >> 12) & 0x3Fu));
			result += static_cast<char>(0x80u | ((codepoint >> 6) & 0x3Fu));
			result += static_cast<char>(0x80u | (codepoint & 0x3Fu));
		}
	}

	return result;
}

char32_t ToLowerCodepoint(const char32_t codepoint)
{
	if (codepoint >= U'A' && codepoint <= U'Z')
	{
		return codepoint + 32;
	}
	if (codepoint >= 0x0410 && codepoint <= 0x042F)
	{
		return codepoint + 0x20;
	}
	if (codepoint == 0x0401)
	{
		return 0x0451;
	}

	return codepoint;
}

std::string ToLowerUtf8(const std::string& value)
{
	std::vector<char32_t> codepoints = DecodeUtf8(value);
	for (char32_t& codepoint : codepoints)
	{
		codepoint = ToLowerCodepoint(codepoint);
	}

	return EncodeUtf8(codepoints);
}

bool HasDigit(const std::string& value)
{
	return std::ranges::any_of(value, [](const unsigned char ch) { return std::isdigit(ch) != 0; });
}

bool LooksLikeEmail(const std::string& value)
{
	const auto atPos = value.find('@');
	if (atPos == std::string::npos)
	{
		return false;
	}

	return value.find('.', atPos) != std::string::npos;
}

bool LooksLikeTelegram(const std::string& value)
{
	return value.find("t.me/") != std::string::npos || (!value.empty() && value.front() == '@');
}

std::string ExtractTelegramHandle(const std::string& value)
{
	const std::string marker = "t.me/";
	const auto pos = value.find(marker);
	if (pos != std::string::npos)
	{
		return value.substr(pos + marker.size());
	}

	if (!value.empty() && value.front() == '@')
	{
		return value.substr(1);
	}

	return value;
}

void AppendNote(RawContact& contact, const std::string& value)
{
	contact.notes = contact.notes.empty() ? value : contact.notes + "; " + value;
}

void ApplyMixedContactValue(const std::string& value, RawContact& contact)
{
	if (LooksLikeEmail(value))
	{
		contact.channels.push_back(ContactChannel{ChannelType::Email, value});
	}
	else if (LooksLikeTelegram(value))
	{
		contact.channels.push_back(ContactChannel{ChannelType::TelegramUsername, ExtractTelegramHandle(value)});
	}
	else if (HasDigit(value))
	{
		contact.channels.push_back(ContactChannel{ChannelType::Phone, value});
	}
	else
	{
		AppendNote(contact, value);
	}
}
} // namespace

ColumnKind ContactRowMapper::ClassifyHeader(const std::string& header)
{
	const std::string lower = ToLowerUtf8(TrimCopy(header));

	if (lower.empty())
	{
		return ColumnKind::Unknown;
	}
	if (lower == "фио" || lower.starts_with("ф.и.о") || lower == "name" || lower == "full name")
	{
		return ColumnKind::FullName;
	}
	if (lower.starts_with("имя") || lower == "firstname")
	{
		return ColumnKind::FirstName;
	}
	if (lower.starts_with("фамил") || lower == "surname" || lower == "lastname")
	{
		return ColumnKind::LastName;
	}
	if (lower.starts_with("сфер") || lower.starts_with("роль") || lower.starts_with("должн")
		|| lower.starts_with("направлен") || lower == "role" || lower == "position")
	{
		return ColumnKind::Role;
	}
	if (lower.starts_with("город") || lower.starts_with("регион") || lower.starts_with("локац")
		|| lower == "location" || lower == "city")
	{
		return ColumnKind::Location;
	}
	if (lower.starts_with("телефон") || lower == "phone" || lower == "tel")
	{
		return ColumnKind::Phone;
	}
	if (lower.starts_with("почта") || lower.starts_with("email") || lower.starts_with("e-mail")
		|| lower.starts_with("мейл"))
	{
		return ColumnKind::Email;
	}
	if (lower.starts_with("telegram") || lower.starts_with("телеграм"))
	{
		return ColumnKind::Telegram;
	}
	if (lower.starts_with("контакт") || lower == "contact")
	{
		return ColumnKind::Contact;
	}
	if (lower.starts_with("заметк") || lower.starts_with("коммент") || lower.starts_with("навык")
		|| lower.starts_with("опыт") || lower.starts_with("описан") || lower.starts_with("ссылк")
		|| lower.starts_with("источник") || lower == "notes" || lower == "skills" || lower == "link")
	{
		return ColumnKind::Notes;
	}

	return ColumnKind::Unknown;
}

void ContactRowMapper::ApplyColumnValue(const ColumnKind kind, const std::string& value, RawContact& contact)
{
	const std::string trimmed = TrimCopy(value);
	if (trimmed.empty())
	{
		return;
	}

	switch (kind)
	{
	case ColumnKind::FullName:
		if (contact.firstName.empty() && contact.lastName.empty())
		{
			contact.firstName = trimmed;
		}
		break;
	case ColumnKind::FirstName:
		contact.firstName = trimmed;
		break;
	case ColumnKind::LastName:
		contact.lastName = trimmed;
		break;
	case ColumnKind::Role:
		contact.role = contact.role.empty() ? trimmed : contact.role + ", " + trimmed;
		break;
	case ColumnKind::Location:
		contact.location = trimmed;
		break;
	case ColumnKind::Phone:
		contact.channels.push_back(ContactChannel{ChannelType::Phone, trimmed});
		break;
	case ColumnKind::Email:
		contact.channels.push_back(ContactChannel{ChannelType::Email, trimmed});
		break;
	case ColumnKind::Telegram:
		contact.channels.push_back(ContactChannel{ChannelType::TelegramUsername, ExtractTelegramHandle(trimmed)});
		break;
	case ColumnKind::Contact:
		ApplyMixedContactValue(trimmed, contact);
		break;
	case ColumnKind::Notes:
		AppendNote(contact, trimmed);
		break;
	case ColumnKind::Unknown:
		break;
	}
}
