#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace TgBot
{
class Api;
} // namespace TgBot

// Отправляет только готовые документы. Не хранит список отправленных сообщений и
// структурно не способен попасть в список на удаление — артефакты нельзя стереть по ошибке.
class ArtifactSender
{
public:
	explicit ArtifactSender(const TgBot::Api* api);

	void SendDocument(
		std::int64_t chatId,
		const std::filesystem::path& path,
		const std::string& caption,
		const std::string& mimeType) const;

private:
	const TgBot::Api* m_api;
};
