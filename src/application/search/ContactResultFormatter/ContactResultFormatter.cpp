#include "ContactResultFormatter.hpp"

#include <algorithm>

namespace
{
constexpr auto NoResultsText = "Никого не нашёл по такому запросу. Попробуйте переформулировать — например, "
								"добавьте роль или город.";
}

std::string ContactResultFormatter::FindChannelValue(const Contact& contact, const ChannelType type)
{
	const auto it = std::ranges::find_if(contact.channels, [type](const ContactChannel& channel) {
		return channel.type == type;
	});

	return it == contact.channels.end() ? "" : it->value;
}

std::string ContactResultFormatter::FormatOne(const std::size_t index, const Contact& contact)
{
	std::string text = std::to_string(index) + ". " + contact.canonicalName + "\n";

	if (!contact.role.empty() || !contact.location.empty())
	{
		text += contact.role;
		if (!contact.role.empty() && !contact.location.empty())
		{
			text += ", ";
		}
		text += contact.location;
		text += "\n";
	}

	const std::string username = FindChannelValue(contact, ChannelType::TelegramUsername);
	if (!username.empty())
	{
		text += "Telegram: @" + username + "\n";
	}

	const std::string phone = FindChannelValue(contact, ChannelType::Phone);
	if (!phone.empty())
	{
		text += "Телефон: " + phone + "\n";
	}

	return text;
}

std::string ContactResultFormatter::Format(const std::vector<SearchResult>& results)
{
	if (results.empty())
	{
		return NoResultsText;
	}

	std::string text;
	for (std::size_t index = 0; index < results.size(); ++index)
	{
		if (index > 0)
		{
			text += "\n";
		}
		text += FormatOne(index + 1, results[index].contact);
	}

	return text;
}
