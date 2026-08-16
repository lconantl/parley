#pragma once

#include "ISemanticClusterSplitter.hpp"

class DeepSeekSemanticSplitter final : public ISemanticClusterSplitter
{
public:
	DeepSeekSemanticSplitter();
	~DeepSeekSemanticSplitter() override;

	std::vector<SemanticClusterSplit> Split(
		const std::vector<MessageCluster>& clusters,
		const std::vector<RawMessage>& rawMessages) const override;
};