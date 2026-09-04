#pragma once

#include "ConversationSession.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

class ConversationRegistry
{
public:
	ConversationSession Get(std::int64_t userId, std::int64_t chatId);

	void SetStep(std::int64_t userId, std::int64_t chatId, ConversationStep step);
	void SetPrompt(std::int64_t userId, std::int64_t chatId, std::int32_t promptMessageId);
	void SetSelection(std::int64_t userId, std::int64_t chatId, DocumentSelection selection);
	void SetAnonymize(std::int64_t userId, std::int64_t chatId, bool anonymize);
	void Track(std::int64_t userId, std::int64_t chatId, std::int32_t messageId);

	bool TryEnterProcessing(std::int64_t userId, std::int64_t chatId);
	bool IsProcessing(std::int64_t userId) const;

	std::vector<MessageRef> TakeTrackedMessages(std::int64_t userId, std::int64_t chatId);
	void ResetToIdle(std::int64_t userId, std::int64_t chatId);

private:
	struct Entry
	{
		std::mutex mutex;
		ConversationSession session;
	};

	Entry& GetEntry(std::int64_t userId, std::int64_t chatId);

	mutable std::mutex m_mapMutex;
	std::unordered_map<std::int64_t, std::unique_ptr<Entry>> m_entries;
};
