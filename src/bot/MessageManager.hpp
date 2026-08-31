#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace TgBot
{
class Api;
} // namespace TgBot

struct MessageRef
{
	std::int64_t chatId = 0;
	std::int32_t messageId = 0;
};

class MessageManager
{
public:
	explicit MessageManager(const TgBot::Api* api);

	void TrackMessage(std::int64_t chatId, std::int32_t messageId);
	void SendStatus(std::int64_t chatId, const std::string& statusText);
	void SendText(std::int64_t chatId, const std::string& text) const;
	void SendDocument(
		std::int64_t chatId,
		const std::filesystem::path& path,
		const std::string& caption) const;
	void DeleteTrackedMessages();

private:
	const TgBot::Api* m_api;
	std::vector<MessageRef> m_trackedMessages;
};