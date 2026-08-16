#pragma once

#include "MessageCluster.hpp"
#include "RawMessage.hpp"
#include <vector>

class IConversationGrouper
{
public:
	virtual ~IConversationGrouper() = default;

	virtual std::vector<MessageCluster> Group(const std::vector<RawMessage>& messages) const = 0;
};