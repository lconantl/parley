#pragma once

#include "IConversationGrouper.hpp"

class ConversationGrouper final : public IConversationGrouper
{
public:
	ConversationGrouper();
	~ConversationGrouper() override;

	std::vector<MessageCluster> Group(const std::vector<RawMessage>& messages) const override;
};