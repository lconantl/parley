#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct MessageRef
{
	int64_t chatId;
	int32_t messageId;
};

class MessageManager
{
public:
	explicit MessageManager(void* api);

	void TrackMessage(int64_t chatId, int32_t messageId);
	void DeleteTrackedMessages();
	void SendStatus(int64_t chatId, const std::string& statusText);

private:
	void* m_api;
	std::vector<MessageRef> m_trackedMessages;
};