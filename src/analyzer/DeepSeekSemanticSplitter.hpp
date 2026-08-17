#pragma once

#include "ISemanticClusterSplitter.hpp"
#include "ai/IAIClient.hpp"
#include <memory>

class DeepSeekSemanticSplitter final : public ISemanticClusterSplitter
{
public:
	explicit DeepSeekSemanticSplitter(std::unique_ptr<IAIClient> aiClient);
	~DeepSeekSemanticSplitter() override;

	std::vector<SemanticClusterSplit> Split(
		const std::vector<MessageCluster>& clusters,
		const std::vector<RawMessage>& rawMessages) const override;

private:
	std::unique_ptr<IAIClient> m_aiClient;
};