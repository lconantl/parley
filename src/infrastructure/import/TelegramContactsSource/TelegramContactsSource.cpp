#include "TelegramContactsSource.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

namespace
{
void AssertIsFileOpen(const std::ifstream& file, const std::filesystem::path& path)
{
	if (!file.is_open())
	{
		throw std::runtime_error("[TelegramContactsSource] Не удалось открыть файл: " + path.string());
	}
}

const nlohmann::json& ResolveContactList(const nlohmann::json& root)
{
	if (root.contains("list") && root.at("list").is_array())
	{
		return root.at("list");
	}

	if (root.contains("contacts") && root.at("contacts").contains("list"))
	{
		return root.at("contacts").at("list");
	}

	throw std::runtime_error("[TelegramContactsSource] В файле не найден список контактов ('list')");
}

std::string GetStringField(const nlohmann::json& entry, const std::string& key)
{
	if (!entry.contains(key) || !entry.at(key).is_string())
	{
		return "";
	}

	return entry.at(key).get<std::string>();
}
} // namespace

std::vector<RawContact> TelegramContactsSource::Load(const std::filesystem::path& path) const
{
	std::ifstream file(path);
	AssertIsFileOpen(file, path);

	nlohmann::json root;
	file >> root;

	const nlohmann::json& list = ResolveContactList(root);

	std::vector<RawContact> contacts;
	for (const auto& entry : list)
	{
		RawContact raw;
		raw.source = ContactSource::TelegramExport;
		raw.firstName = GetStringField(entry, "first_name");
		raw.lastName = GetStringField(entry, "last_name");

		const std::string phone = GetStringField(entry, "phone_number");
		if (!phone.empty())
		{
			raw.channels.push_back(ContactChannel{ChannelType::Phone, phone});
		}

		contacts.push_back(raw);
	}

	return contacts;
}
