#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct ClusterFeatures
{
	size_t messageCount = 0;
	size_t participantCount = 0;
	int64_t durationSeconds = 0;
	size_t replyCount = 0;
	size_t forwardCount = 0;
	size_t fileCount = 0;
	size_t photoCount = 0;
	size_t contactCount = 0;
	size_t linkCount = 0;
	int64_t maxGapSeconds = 0;
	double averageGapSeconds = 0.0;
	int64_t medianGapSeconds = 0;
	size_t connectionCount = 0;
	std::unordered_map<std::string, size_t> connectionReasons;
};

struct MessageCluster
{
	int64_t id = 0;
	std::vector<int64_t> messageIds;
	std::optional<int64_t> rootMessageId;
	std::optional<int64_t> telegramThreadRootId;
	std::string startDate;
	std::string endDate;
	std::vector<std::string> participants;
	std::vector<int64_t> replyMessageIds;
	std::vector<int64_t> forwardedMessageIds;
	std::vector<std::string> filePaths;
	std::vector<std::string> contactVcards;
	ClusterFeatures features;
};