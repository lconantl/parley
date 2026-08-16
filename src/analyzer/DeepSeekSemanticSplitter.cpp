#include "DeepSeekSemanticSplitter.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace
{
void AssertIsConditionMet(const bool condition, const std::string& errorMessage)
{
	if (!condition)
	{
		throw std::runtime_error(errorMessage);
	}
}

std::unordered_map<int64_t, const RawMessage*> BuildMessageIndex(const std::vector<RawMessage>& messages)
{
	std::unordered_map<int64_t, const RawMessage*> index;
	for (const auto& msg : messages)
	{
		if (msg.id.has_value())
		{
			index[msg.id.value()] = &msg;
		}
	}
	return index;
}

std::string ExtractTextFast(const RawMessage& msg)
{
	if (msg.text.is_string())
	{
		return msg.text.get<std::string>();
	}
	if (msg.text.is_array() && !msg.text.empty() && msg.text[0].is_string())
	{
		return msg.text[0].get<std::string>();
	}
	return "";
}

nlohmann::json SerializeMessageForAi(const RawMessage& msg)
{
	nlohmann::json node;
	node["id"] = msg.id.value_or(0);
	node["date"] = msg.date.value_or("");
	node["from"] = msg.from.value_or("");
	node["text"] = ExtractTextFast(msg);

	if (msg.replyToMessageId.has_value())
	{
		node["reply_to_message_id"] = msg.replyToMessageId.value();
	}
	else
	{
		node["reply_to_message_id"] = nullptr;
	}

	if (msg.forwardedFrom.has_value())
	{
		node["forwarded_from"] = msg.forwardedFrom.value();
	}
	else
	{
		node["forwarded_from"] = nullptr;
	}

	node["entities"] = nlohmann::json::array();
	for (const auto& entity : msg.textEntities)
	{
		nlohmann::json entityNode;
		entityNode["type"] = entity.type;
		entityNode["text"] = entity.text;
		node["entities"].push_back(std::move(entityNode));
	}

	node["raw_json"] = msg.rawJson;
	return node;
}

std::string PrepareAiPrompt(const MessageCluster& cluster, const std::unordered_map<int64_t, const RawMessage*>& index)
{
	nlohmann::json payload;
	payload["cluster_id"] = cluster.id;
	payload["messages"] = nlohmann::json::array();

	for (const int64_t msgId : cluster.messageIds)
	{
		if (index.contains(msgId))
		{
			payload["messages"].push_back(SerializeMessageForAi(*index.at(msgId)));
		}
	}

	return payload.dump();
}

std::string PerformNetworkRequestMock(const std::string& requestJson)
{
	const nlohmann::json req = nlohmann::json::parse(requestJson);
	nlohmann::json res;
	res["cluster_id"] = req["cluster_id"];
	res["split"] = false;

	nlohmann::json comp;
	comp["id"] = std::to_string(req["cluster_id"].get<int64_t>()) + "-1";
	comp["message_ids"] = nlohmann::json::array();

	for (const auto& m : req["messages"])
	{
		comp["message_ids"].push_back(m["id"]);
	}

	res["components"] = nlohmann::json::array({comp});
	return res.dump();
}

SemanticClusterSplit ParseAiResponse(const std::string& responseJson)
{
	SemanticClusterSplit split;
	split.rawAiResponse = responseJson;

	nlohmann::json root;
	try
	{
		root = nlohmann::json::parse(responseJson);
	}
	catch (const nlohmann::json::parse_error&)
	{
		AssertIsConditionMet(false, "Ответ API не является валидным JSON");
	}

	AssertIsConditionMet(root.contains("cluster_id") && root["cluster_id"].is_number(), "Отсутствует корректное поле cluster_id");
	AssertIsConditionMet(root.contains("split") && root["split"].is_boolean(), "Отсутствует корректное поле split");
	AssertIsConditionMet(root.contains("components") && root["components"].is_array(), "Отсутствует массив components");

	split.clusterId = root["cluster_id"].get<int64_t>();
	split.split = root["split"].get<bool>();

	for (const auto& compNode : root["components"])
	{
		AssertIsConditionMet(compNode.contains("id") && compNode["id"].is_string(), "Компонент не содержит строкового id");
		AssertIsConditionMet(compNode.contains("message_ids") && compNode["message_ids"].is_array(), "Компонент не содержит массив message_ids");

		SemanticComponent component;
		component.id = compNode["id"].get<std::string>();

		for (const auto& idNode : compNode["message_ids"])
		{
			AssertIsConditionMet(idNode.is_number(), "Идентификатор сообщения должен быть числом");
			component.messageIds.push_back(idNode.get<int64_t>());
		}

		split.components.push_back(std::move(component));
	}

	return split;
}

void ValidateSplitAgainstOriginal(const SemanticClusterSplit& split, const MessageCluster& cluster)
{
	AssertIsConditionMet(split.clusterId == cluster.id, "ID кластера в ответе не совпадает с запрошенным");

	const std::unordered_set expectedIds(cluster.messageIds.begin(), cluster.messageIds.end());
	std::unordered_set<int64_t> actualIds;
	std::unordered_set<std::string> componentIds;

	for (const auto& comp : split.components)
	{
		AssertIsConditionMet(!componentIds.contains(comp.id), "Нейросеть вернула дублирующиеся ID компонентов");
		componentIds.insert(comp.id);

		for (const int64_t msgId : comp.messageIds)
		{
			AssertIsConditionMet(expectedIds.contains(msgId), "Нейросеть придумала несуществующий ID сообщения (галлюцинация)");
			actualIds.insert(msgId);
		}
	}

	AssertIsConditionMet(actualIds.size() == expectedIds.size(), "Нейросеть потеряла часть сообщений при разделении (существуют 'потеряшки')");
}
} // namespace

DeepSeekSemanticSplitter::DeepSeekSemanticSplitter()
{
}

DeepSeekSemanticSplitter::~DeepSeekSemanticSplitter()
{
}

std::vector<SemanticClusterSplit> DeepSeekSemanticSplitter::Split(
	const std::vector<MessageCluster>& clusters,
	const std::vector<RawMessage>& rawMessages) const
{
	const auto index = BuildMessageIndex(rawMessages);
	std::vector<SemanticClusterSplit> results;
	results.reserve(clusters.size());

	for (const auto& cluster : clusters)
	{
		if (cluster.messageIds.size() < 2)
		{
			continue;
		}

		const std::string prompt = PrepareAiPrompt(cluster, index);
		const std::string response = PerformNetworkRequestMock(prompt);
		const SemanticClusterSplit split = ParseAiResponse(response);

		ValidateSplitAgainstOriginal(split, cluster);
		results.push_back(split);
	}

	return results;
}