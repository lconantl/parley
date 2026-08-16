#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct SemanticComponent
{
	std::string id;
	std::vector<int64_t> messageIds;
};

struct SemanticClusterSplit
{
	int64_t clusterId = 0;
	bool split = false;
	std::vector<SemanticComponent> components;
	std::string rawAiResponse;
};