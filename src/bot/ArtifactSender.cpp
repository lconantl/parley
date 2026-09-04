#include "ArtifactSender.hpp"

#include <stdexcept>
#include <tgbot/tgbot.h>

namespace
{
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
} // namespace

ArtifactSender::ArtifactSender(const TgBot::Api* api)
	: m_api(api)
{
	AssertIsApiAvailable(m_api);
}

void ArtifactSender::SendDocument(
	const std::int64_t chatId,
	const std::filesystem::path& path,
	const std::string& caption,
	const std::string& mimeType) const
{
	AssertIsFileAvailable(path);

	const auto document = TgBot::InputFile::fromFile(path.string(), mimeType);
	m_api->sendDocument(chatId, document, "", caption);
}
