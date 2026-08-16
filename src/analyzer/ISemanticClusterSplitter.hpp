#pragma once

#include "MessageCluster.hpp"
#include "RawMessage.hpp"
#include "SemanticSplit.hpp"
#include <vector>

class ISemanticClusterSplitter
{
public:
	virtual ~ISemanticClusterSplitter() = default;

	virtual std::vector<SemanticClusterSplit> Split(
		const std::vector<MessageCluster>& clusters,
		const std::vector<RawMessage>& rawMessages) const = 0;
};