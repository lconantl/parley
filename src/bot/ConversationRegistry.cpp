#include "ConversationRegistry.hpp"

#include <utility>

ConversationRegistry::Entry& ConversationRegistry::GetEntry(
	const std::int64_t userId,
	const std::int64_t chatId)
{
	std::lock_guard lock(m_mapMutex);

	const auto iterator = m_entries.find(userId);
	if (iterator != m_entries.end())
	{
		return *iterator->second;
	}

	auto entry = std::make_unique<Entry>();
	entry->session.chatId = chatId;

	return *m_entries.emplace(userId, std::move(entry)).first->second;
}

ConversationSession ConversationRegistry::Get(const std::int64_t userId, const std::int64_t chatId)
{
	Entry& entry = GetEntry(userId, chatId);
	std::lock_guard lock(entry.mutex);

	return entry.session;
}

void ConversationRegistry::SetStep(
	const std::int64_t userId,
	const std::int64_t chatId,
	const ConversationStep step)
{
	Entry& entry = GetEntry(userId, chatId);
	std::lock_guard lock(entry.mutex);

	entry.session.step = step;
}

void ConversationRegistry::SetPrompt(
	const std::int64_t userId,
	const std::int64_t chatId,
	const std::int32_t promptMessageId)
{
	Entry& entry = GetEntry(userId, chatId);
	std::lock_guard lock(entry.mutex);

	entry.session.promptMessageId = promptMessageId;
}

void ConversationRegistry::SetSelection(
	const std::int64_t userId,
	const std::int64_t chatId,
	const DocumentSelection selection)
{
	Entry& entry = GetEntry(userId, chatId);
	std::lock_guard lock(entry.mutex);

	entry.session.selection = selection;
}

void ConversationRegistry::SetAnonymize(
	const std::int64_t userId,
	const std::int64_t chatId,
	const bool anonymize)
{
	Entry& entry = GetEntry(userId, chatId);
	std::lock_guard lock(entry.mutex);

	entry.session.anonymize = anonymize;
}

void ConversationRegistry::Track(
	const std::int64_t userId,
	const std::int64_t chatId,
	const std::int32_t messageId)
{
	Entry& entry = GetEntry(userId, chatId);
	std::lock_guard lock(entry.mutex);

	entry.session.trackedMessages.push_back({chatId, messageId});
}

bool ConversationRegistry::TryEnterProcessing(const std::int64_t userId, const std::int64_t chatId)
{
	Entry& entry = GetEntry(userId, chatId);
	std::lock_guard lock(entry.mutex);

	if (entry.session.step == ConversationStep::Processing)
	{
		return false;
	}

	entry.session.step = ConversationStep::Processing;

	return true;
}

bool ConversationRegistry::IsProcessing(const std::int64_t userId) const
{
	std::lock_guard mapLock(m_mapMutex);

	const auto iterator = m_entries.find(userId);
	if (iterator == m_entries.end())
	{
		return false;
	}

	std::lock_guard lock(iterator->second->mutex);

	return iterator->second->session.step == ConversationStep::Processing;
}

std::vector<MessageRef> ConversationRegistry::TakeTrackedMessages(
	const std::int64_t userId,
	const std::int64_t chatId)
{
	Entry& entry = GetEntry(userId, chatId);
	std::lock_guard lock(entry.mutex);

	std::vector<MessageRef> taken;
	std::swap(taken, entry.session.trackedMessages);

	return taken;
}

void ConversationRegistry::ResetToIdle(const std::int64_t userId, const std::int64_t chatId)
{
	Entry& entry = GetEntry(userId, chatId);
	std::lock_guard lock(entry.mutex);

	entry.session.step = ConversationStep::Idle;
	entry.session.selection = DocumentSelection::None;
	entry.session.anonymize = false;
	entry.session.promptMessageId = 0;
}
