#include "VCardContactsSource.hpp"

#include <fstream>
#include <stdexcept>

namespace
{
void AssertIsFileOpen(const std::ifstream& file, const std::filesystem::path& path)
{
	if (!file.is_open())
	{
		throw std::runtime_error("[VCardContactsSource] Не удалось открыть файл: " + path.string());
	}
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

bool KeyMatches(const std::string& key, const std::string& name)
{
	return key == name || key.starts_with(name + ";");
}

std::vector<std::string> SplitBySemicolon(const std::string& value)
{
	std::vector<std::string> parts;
	std::string current;

	for (const char ch : value)
	{
		if (ch == ';')
		{
			parts.push_back(current);
			current.clear();
		}
		else
		{
			current += ch;
		}
	}
	parts.push_back(current);

	return parts;
}

void ApplyLine(const std::string& line, RawContact& contact)
{
	const auto colonPos = line.find(':');
	if (colonPos == std::string::npos)
	{
		return;
	}

	const std::string key = line.substr(0, colonPos);
	const std::string value = TrimCopy(line.substr(colonPos + 1));

	if (KeyMatches(key, "N"))
	{
		const auto parts = SplitBySemicolon(value);
		if (!parts.empty())
		{
			contact.lastName = parts[0];
		}
		if (parts.size() > 1)
		{
			contact.firstName = parts[1];
		}
	}
	else if (KeyMatches(key, "FN") && contact.firstName.empty() && contact.lastName.empty())
	{
		contact.firstName = value;
	}
	else if (KeyMatches(key, "TEL"))
	{
		if (!value.empty())
		{
			contact.channels.push_back(ContactChannel{ChannelType::Phone, value});
		}
	}
	else if (KeyMatches(key, "NOTE"))
	{
		contact.notes = contact.notes.empty() ? value : contact.notes + " " + value;
	}
}
} // namespace

std::vector<RawContact> VCardContactsSource::Load(const std::filesystem::path& path) const
{
	std::ifstream file(path);
	AssertIsFileOpen(file, path);

	std::vector<RawContact> contacts;
	RawContact current;
	bool insideCard = false;

	std::string rawLine;
	while (std::getline(file, rawLine))
	{
		const std::string line = TrimCopy(rawLine);

		if (line == "BEGIN:VCARD")
		{
			current = RawContact{};
			current.source = ContactSource::PhoneBook;
			insideCard = true;
			continue;
		}

		if (line == "END:VCARD")
		{
			if (insideCard)
			{
				contacts.push_back(current);
			}
			insideCard = false;
			continue;
		}

		if (!insideCard || line.empty())
		{
			continue;
		}

		ApplyLine(line, current);
	}

	return contacts;
}
