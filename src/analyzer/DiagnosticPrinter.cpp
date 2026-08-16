#include "DiagnosticPrinter.hpp"
#include <algorithm>
#include <iostream>
#include <ranges>
#include <string>
#include <unordered_map>

namespace
{
std::string FormatDuration(const int64_t seconds)
{
	const int64_t hours = seconds / 3600;
	const int64_t minutes = seconds % 3600 / 60;
	const int64_t secs = seconds % 60;
	std::string result;

	if (hours > 0)
	{
		result += std::to_string(hours) + "h ";
	}
	if (minutes > 0 || hours > 0)
	{
		result += std::to_string(minutes) + "m ";
	}
	if (secs > 0 || result.empty())
	{
		result += std::to_string(secs) + "s";
	}
	return result;
}

std::string ExtractTextPreview(const RawMessage& message)
{
	if (message.text.is_string())
	{
		auto text = message.text.get<std::string>();
		if (text.length() > 100)
		{
			text = text.substr(0, 97) + "...";
		}
		std::erase(text, '\n');
		std::erase(text, '\r');
		return text.empty() ? "[Empty String]" : text;
	}
	if (message.text.is_array() && !message.text.empty() && message.text[0].is_string())
	{
		auto text = message.text[0].get<std::string>();
		std::erase(text, '\n');
		std::erase(text, '\r');
		return text + " ...";
	}
	return "[Complex Text / Media]";
}

void PrintClusterSingle(
	const MessageCluster& cluster,
	const std::unordered_map<int64_t, const RawMessage*>& idToMsg)
{
	std::cout << "Cluster #" << cluster.id << "\n\n";
	std::cout << "Messages: " << cluster.features.messageCount << "\n";
	std::cout << "Duration: " << FormatDuration(cluster.features.durationSeconds) << "\n";
	std::cout << "Participants: " << cluster.features.participantCount << "\n\n";

	std::cout << "Time gaps:\n";
	std::cout << "  median: " << FormatDuration(cluster.features.medianGapSeconds) << "\n";
	std::cout << "  max: " << FormatDuration(cluster.features.maxGapSeconds) << "\n\n";

	std::cout << "Relations:\n";
	for (const auto& [reason, count] : cluster.features.connectionReasons)
	{
		std::cout << "  " << reason << ": " << count << "\n";
	}
	if (cluster.features.connectionReasons.empty())
	{
		std::cout << "  no sufficiently strong relations\n";
	}

	if (cluster.rootMessageId.has_value() && idToMsg.contains(cluster.rootMessageId.value()))
	{
		const int64_t rootId = cluster.rootMessageId.value();
		std::cout << "\nRoot:\n[message " << rootId << "] \""
				  << ExtractTextPreview(*idToMsg.at(rootId)) << "\"\n";
	}

	std::cout << "\nMessages:\n";
	for (const int64_t id : cluster.messageIds | std::views::take(10))
	{
		if (idToMsg.contains(id))
		{
			std::cout << id << " - " << ExtractTextPreview(*idToMsg.at(id)) << "\n";
		}
	}

	if (cluster.messageIds.size() > 10)
	{
		std::cout << "...\n";
	}
	std::cout << "----------------------------------------\n";
}
} // namespace

namespace DiagnosticPrinter
{
void PrintClustersTop(std::vector<MessageCluster>& clusters, const std::vector<RawMessage>& rawMessages)
{
	std::ranges::sort(clusters, std::greater{}, [](const MessageCluster& c) {
		return c.features.messageCount;
	});

	std::unordered_map<int64_t, const RawMessage*> idToMsg;
	for (const auto& msg : rawMessages)
	{
		if (msg.id.has_value())
		{
			idToMsg[msg.id.value()] = &msg;
		}
	}

	for (const auto& cluster : clusters | std::views::take(20))
	{
		PrintClusterSingle(cluster, idToMsg);
	}
}

void PrintClusterById(const std::vector<MessageCluster>& clusters, const std::vector<RawMessage>& rawMessages, const int64_t targetId)
{
	std::unordered_map<int64_t, const RawMessage*> idToMsg;
	for (const auto& msg : rawMessages)
	{
		if (msg.id.has_value())
		{
			idToMsg[msg.id.value()] = &msg;
		}
	}

	for (const auto& cluster : clusters)
	{
		if (cluster.id == targetId)
		{
			PrintClusterSingle(cluster, idToMsg);
			return;
		}
	}
	std::cout << "Кластер с ID " << targetId << " не найден.\n";
}

void PrintClusterDiagnostics(const std::vector<MessageCluster>& clusters, const size_t totalMessages)
{
	size_t singletons = 0;
	size_t size2to5 = 0;
	size_t size6to20 = 0;
	size_t size21to100 = 0;
	size_t sizeOver100 = 0;
	size_t largestSegmentSize = 0;
	int64_t longestSegmentDuration = 0;

	std::vector<int64_t> durations;
	durations.reserve(clusters.size());

	for (const auto& cluster : clusters)
	{
		const size_t count = cluster.features.messageCount;
		if (count == 1)
			singletons++;
		else if (count <= 5)
			size2to5++;
		else if (count <= 20)
			size6to20++;
		else if (count <= 100)
			size21to100++;
		else
			sizeOver100++;

		if (count > largestSegmentSize) largestSegmentSize = count;
		if (cluster.features.durationSeconds > longestSegmentDuration)
		{
			longestSegmentDuration = cluster.features.durationSeconds;
		}

		durations.push_back(cluster.features.durationSeconds);
	}

	std::ranges::sort(durations);
	const int64_t medianDuration = durations.empty() ? 0 : durations[durations.size() / 2];
	const int64_t p90Duration = durations.empty() ? 0 : durations[static_cast<size_t>(durations.size() * 0.9)];

	std::cout << "DIAGNOSTICS REPORT\n";
	std::cout << "Total messages: " << totalMessages << "\n";
	std::cout << "Total segments: " << clusters.size() << "\n\n";

	std::cout << "Size Distribution:\n";
	std::cout << "  Singletons:       " << singletons << "\n";
	std::cout << "  2-5 messages:     " << size2to5 << "\n";
	std::cout << "  6-20 messages:    " << size6to20 << "\n";
	std::cout << "  21-100 messages:  " << size21to100 << "\n";
	std::cout << "  >100 messages:    " << sizeOver100 << "\n\n";

	std::cout << "Extremes:\n";
	std::cout << "  Largest segment:  " << largestSegmentSize << " messages\n";
	std::cout << "  Longest segment:  " << FormatDuration(longestSegmentDuration) << "\n\n";

	std::cout << "Durations:\n";
	std::cout << "  Median duration:  " << FormatDuration(medianDuration) << "\n";
	std::cout << "  P90 duration:     " << FormatDuration(p90Duration) << "\n";
}

void PrintSemanticSplits(const std::vector<SemanticClusterSplit>& splits)
{
	std::cout << "SEMANTIC SPLIT RESULTS" << std::endl;
	std::cout << "Processed clusters: " << splits.size() << std::endl;

	for (const auto& split : splits | std::views::take(5))
	{
		std::cout << "Cluster #" << split.clusterId << std::endl;
		std::cout << "Status: " << (split.split ? "Splitted" : "Intact") << std::endl;

		for (const auto& comp : split.components)
		{
			std::cout << "  Component " << comp.id << " (" << comp.messageIds.size() << " msgs)" << std::endl;
		}

		std::cout << "-------------------" << std::endl;
	}
}
} // namespace DiagnosticPrinter