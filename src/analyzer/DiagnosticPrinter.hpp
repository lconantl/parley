#pragma once

#include "MessageCluster.hpp"
#include "RawMessage.hpp"
#include "SemanticSplit.hpp"
#include <vector>

namespace DiagnosticPrinter
{
void PrintClustersTop(std::vector<MessageCluster>& clusters, const std::vector<RawMessage>& rawMessages);
void PrintClusterById(const std::vector<MessageCluster>& clusters, const std::vector<RawMessage>& rawMessages, int64_t targetId);
void PrintClusterDiagnostics(const std::vector<MessageCluster>& clusters, size_t totalMessages);
void PrintSemanticSplits(const std::vector<SemanticClusterSplit>& splits);
} // namespace DiagnosticPrinter