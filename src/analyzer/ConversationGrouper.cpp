#include "ConversationGrouper.hpp"
#include <algorithm>
#include <charconv>
#include <stdexcept>
#include <unordered_set>

namespace
{
void AssertIsNotEmpty(const size_t count)
{
	if (count == 0)
	{
		throw std::runtime_error("Список сообщений для группировки не может быть пустым");
	}
}

int64_t ExtractTimestamp(const RawMessage& message)
{
	if (!message.dateUnixtime.has_value())
	{
		return 0;
	}

	int64_t timestamp = 0;
	const auto& str = message.dateUnixtime.value();

	if (auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), timestamp); ec != std::errc{})
	{
		return 0;
	}

	return timestamp;
}

std::vector<const RawMessage*> SortMessagesByTime(const std::vector<RawMessage>& messages)
{
	std::vector<const RawMessage*> sorted;
	sorted.reserve(messages.size());

	for (const auto& msg : messages)
	{
		if (msg.id.has_value())
		{
			sorted.push_back(&msg);
		}
	}

	std::ranges::sort(sorted, [](const RawMessage* a, const RawMessage* b) {
		return ExtractTimestamp(*a) < ExtractTimestamp(*b);
	});

	return sorted;
}

std::unordered_set<std::string> ExtractStrongEntities(const RawMessage& msg)
{
	std::unordered_set<std::string> entities;
	for (const auto& entity : msg.textEntities)
	{
		if (entity.type == "link" || entity.type == "text_link" || entity.type == "phone" || entity.type == "mention")
		{
			entities.insert(entity.text);
		}
	}
	return entities;
}

struct SegmentContext
{
	std::unordered_set<int64_t> messageIds;
	std::unordered_set<std::string> forwards;
	std::unordered_set<std::string> entities;
	int64_t lastTimestamp = 0;
};

void UpdateSegmentContext(const RawMessage& msg, SegmentContext& ctx)
{
	ctx.messageIds.insert(msg.id.value());

	if (msg.forwardedFrom.has_value())
	{
		ctx.forwards.insert(msg.forwardedFrom.value());
	}

	for (const auto& entity : ExtractStrongEntities(msg))
	{
		ctx.entities.insert(entity);
	}

	ctx.lastTimestamp = ExtractTimestamp(msg);
}

int CalculateTimeScore(const int64_t diff)
{
	if (diff <= 5)
		return 20;
	if (diff <= 30)
		return 15;
	if (diff <= 120)
		return 10;
	if (diff <= 600)
		return 5;
	return 0;
}

bool HasSharedEntities(const RawMessage& msg, const SegmentContext& ctx)
{
	const auto msgEntities = ExtractStrongEntities(msg);
	for (const auto& entity : msgEntities)
	{
		if (ctx.entities.contains(entity))
		{
			return true;
		}
	}
	return false;
}

int EvaluateConnection(const RawMessage& msg, const SegmentContext& ctx, MessageCluster& cluster)
{
	int score = 0;
	const int64_t timestamp = ExtractTimestamp(msg);
	const int64_t diff = std::abs(timestamp - ctx.lastTimestamp);

	if (diff > 7200 || cluster.messageIds.size() > 500)
	{
		return 0;
	}

	const int timeScore = CalculateTimeScore(diff);
	if (timeScore > 0)
	{
		score += timeScore;
		cluster.features.connectionReasons["temporal"]++;
	}

	if (msg.replyToMessageId.has_value() && ctx.messageIds.contains(msg.replyToMessageId.value()))
	{
		score += 100;
		cluster.features.connectionReasons["replies"]++;
	}

	if (msg.forwardedFrom.has_value() && ctx.forwards.contains(msg.forwardedFrom.value()))
	{
		score += 20;
		cluster.features.connectionReasons["forwards"]++;
	}

	if (HasSharedEntities(msg, ctx))
	{
		score += 30;
		cluster.features.connectionReasons["same entities"]++;
	}

	return score;
}

void UpdateTemporalBounds(const RawMessage& msg, MessageCluster& cluster)
{
	if (cluster.startDate.empty() || msg.date.value_or("") < cluster.startDate)
	{
		cluster.startDate = msg.date.value_or("");
		cluster.rootMessageId = msg.id;
	}

	if (cluster.endDate.empty() || msg.date.value_or("") > cluster.endDate)
	{
		cluster.endDate = msg.date.value_or("");
	}
}

void AddMessageToCluster(const RawMessage& msg, MessageCluster& cluster)
{
	cluster.messageIds.push_back(msg.id.value());
	cluster.features.messageCount++;

	if (msg.from.has_value())
	{
		cluster.participants.push_back(msg.from.value());
	}
	if (msg.replyToMessageId.has_value())
	{
		cluster.replyMessageIds.push_back(msg.replyToMessageId.value());
		cluster.features.replyCount++;
	}
	if (msg.forwardedFrom.has_value())
	{
		cluster.forwardedMessageIds.push_back(msg.id.value());
		cluster.features.forwardCount++;
	}
	if (msg.file.has_value())
	{
		cluster.filePaths.push_back(msg.file.value());
		cluster.features.fileCount++;
	}
	if (msg.photo.has_value())
	{
		cluster.filePaths.push_back(msg.photo.value());
		cluster.features.photoCount++;
	}
	if (msg.contactVcard.has_value())
	{
		cluster.contactVcards.push_back(msg.contactVcard.value());
		cluster.features.contactCount++;
	}

	for (const auto& entity : msg.textEntities)
	{
		if (entity.type == "link" || entity.type == "text_link")
		{
			cluster.features.linkCount++;
		}
	}

	UpdateTemporalBounds(msg, cluster);
}

int64_t CalculateMedian(std::vector<int64_t>& values)
{
	if (values.empty())
	{
		return 0;
	}
	const size_t n = values.size() / 2;
	std::ranges::nth_element(values, values.begin() + n);
	return values[n];
}

void FinalizeClusterMetrics(MessageCluster& cluster, const std::unordered_map<int64_t, int64_t>& timestampIndex)
{
	std::unordered_set uniqueParticipants(cluster.participants.begin(), cluster.participants.end());
	cluster.participants.assign(uniqueParticipants.begin(), uniqueParticipants.end());
	cluster.features.participantCount = cluster.participants.size();

	if (cluster.messageIds.size() < 2)
	{
		return;
	}

	std::vector<int64_t> gaps;
	gaps.reserve(cluster.messageIds.size() - 1);
	int64_t totalGap = 0;

	for (size_t i = 1; i < cluster.messageIds.size(); ++i)
	{
		const int64_t ts1 = timestampIndex.at(cluster.messageIds[i - 1]);
		const int64_t ts2 = timestampIndex.at(cluster.messageIds[i]);
		const int64_t gap = std::abs(ts2 - ts1);

		gaps.push_back(gap);
		totalGap += gap;

		if (gap > cluster.features.maxGapSeconds)
		{
			cluster.features.maxGapSeconds = gap;
		}
	}

	cluster.features.averageGapSeconds = static_cast<double>(totalGap) / static_cast<double>(gaps.size());
	cluster.features.medianGapSeconds = CalculateMedian(gaps);

	const int64_t firstTs = timestampIndex.at(cluster.messageIds.front());
	const int64_t lastTs = timestampIndex.at(cluster.messageIds.back());
	cluster.features.durationSeconds = std::abs(lastTs - firstTs);
}

std::vector<MessageCluster> BuildSegments(const std::vector<const RawMessage*>& sortedMessages)
{
	std::vector<MessageCluster> clusters;
	MessageCluster currentCluster;
	SegmentContext currentContext;
	int64_t clusterIdCounter = 1;

	std::unordered_map<int64_t, int64_t> timestampIndex;
	for (const auto* msg : sortedMessages)
	{
		timestampIndex[msg->id.value()] = ExtractTimestamp(*msg);
	}

	for (const auto* msg : sortedMessages)
	{
		if (currentCluster.messageIds.empty())
		{
			currentCluster.id = clusterIdCounter++;
			AddMessageToCluster(*msg, currentCluster);
			UpdateSegmentContext(*msg, currentContext);
			continue;
		}

		const int score = EvaluateConnection(*msg, currentContext, currentCluster);

		if (score >= 5)
		{
			AddMessageToCluster(*msg, currentCluster);
			UpdateSegmentContext(*msg, currentContext);
		}
		else
		{
			FinalizeClusterMetrics(currentCluster, timestampIndex);
			clusters.push_back(std::move(currentCluster));

			currentCluster = MessageCluster();
			currentContext = SegmentContext();

			currentCluster.id = clusterIdCounter++;
			AddMessageToCluster(*msg, currentCluster);
			UpdateSegmentContext(*msg, currentContext);
		}
	}

	if (!currentCluster.messageIds.empty())
	{
		FinalizeClusterMetrics(currentCluster, timestampIndex);
		clusters.push_back(std::move(currentCluster));
	}

	return clusters;
}
} // namespace

ConversationGrouper::ConversationGrouper()
{
}

ConversationGrouper::~ConversationGrouper()
{
}

std::vector<MessageCluster> ConversationGrouper::Group(const std::vector<RawMessage>& messages) const
{
	AssertIsNotEmpty(messages.size());
	const auto sortedMessages = SortMessagesByTime(messages);
	return BuildSegments(sortedMessages);
}