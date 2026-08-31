#include "MessageManager.hpp"
#include <iostream>
#include <stdexcept>
#include <tgbot/tgbot.h>

namespace
{
constexpr std::size_t MaxMessageLength = 3900;
constexpr auto DocumentMimeType = "text/markdown";

void AssertIsApiAvailable(const TgBot::Api* api)
{
	if (api == nullptr)
	{
		throw std::invalid_argument("Клиент Telegram не может быть пустым");
	}
}

void AssertIsFileAvailable(const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path))
	{
		throw std::runtime_error("Файл для отправки не найден: " + path.string());
	}
}

std::size_t FindSplitPosition(const std::string& text, const std::size_t from)
{
	const std::size_t limit = std::min(from + MaxMessageLength, text.size());
	if (limit == text.size())
	{
		return limit;
	}

	const std::size_t lineBreak = text.rfind('\n', limit);
	if (lineBreak != std::string::npos && lineBreak > from)
	{
		return lineBreak + 1;
	}

	std::size_t position = limit;
	while (position > from && (static_cast<unsigned char>(text[position]) & 0xC0) == 0x80)
	{
		--position;
	}

	return position;
}

std::vector<std::string> SplitText(const std::string& text)
{
	std::vector<std::string> parts;
	std::size_t offset = 0;

	while (offset < text.size())
	{
		const std::size_t next = FindSplitPosition(text, offset);
		parts.push_back(text.substr(offset, next - offset));
		offset = next;
	}

	return parts;
}
} // namespace

MessageManager::MessageManager(const TgBot::Api* api)
	: m_api(api)
{
	AssertIsApiAvailable(m_api);
}

void MessageManager::TrackMessage(const std::int64_t chatId, const std::int32_t messageId)
{
	m_trackedMessages.push_back({chatId, messageId});
}

void MessageManager::SendStatus(const std::int64_t chatId, const std::string& statusText)
{
	const auto message = m_api->sendMessage(chatId, statusText);
	TrackMessage(chatId, message->messageId);
}

void MessageManager::SendText(const std::int64_t chatId, const std::string& text) const
{
	if (text.empty())
	{
		return;
	}

	for (const auto& part : SplitText(text))
	{
		m_api->sendMessage(chatId, part);
	}
}

void MessageManager::SendDocument(
	const std::int64_t chatId,
	const std::filesystem::path& path,
	const std::string& caption) const
{
	AssertIsFileAvailable(path);

	const auto document = TgBot::InputFile::fromFile(path.string(), DocumentMimeType);
	m_api->sendDocument(chatId, document, "", caption);
}

void MessageManager::DeleteTrackedMessages()
{
	for (const auto& reference : m_trackedMessages)
	{
		try
		{
			m_api->deleteMessage(reference.chatId, reference.messageId);
		}
		catch (const std::exception& error)
		{
			std::cerr << "Ошибка удаления сообщения: " << error.what() << std::endl;
		}
	}

	m_trackedMessages.clear();
}