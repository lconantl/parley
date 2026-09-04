#pragma once

#include <cstdint>
#include <vector>

struct MessageRef
{
	std::int64_t chatId = 0;
	std::int32_t messageId = 0;
};

enum class ConversationStep
{
	Idle,
	AwaitingDocumentType,
	AwaitingAnonymity,
	AwaitingInn,
	Processing
};

enum class DocumentSelection
{
	None,
	Report,
	Presentation,
	OnePager,
	All
};

struct ConversationSession
{
	std::int64_t chatId = 0;
	ConversationStep step = ConversationStep::Idle;
	DocumentSelection selection = DocumentSelection::None;
	bool anonymize = false;
	std::int32_t promptMessageId = 0;
	std::vector<MessageRef> trackedMessages;
};
