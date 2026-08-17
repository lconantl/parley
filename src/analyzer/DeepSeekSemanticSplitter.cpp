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

std::string GetSystemPrompt()
{
	return "Определи, является ли данный набор сообщений одной смысловой единицей разговора. "
		   "Если нет — раздели его на минимальное количество самостоятельных смысловых компонентов.\n"
		   "ПРАВИЛА:\n"
		   "1. Каждое входное сообщение ОБЯЗАНО быть учтено. Ни один message_id нельзя потерять.\n"
		   "2. Каждый message_id должен находиться хотя бы в одном components[].message_ids ИЛИ в unassigned_message_ids.\n"
		   "3. Одно сообщение разрешено включать в несколько компонентов, если оно относится к нескольким смысловым компонентам.\n"
		   "4. Нельзя придумывать message_id. Нельзя изменять сообщения.\n"
		   "5. Верни строго JSON со структурой: 'cluster_id' (число), 'split' (булевое значение), "
		   "'components' (массив объектов с полями 'id' (строка) и 'message_ids' (массив чисел)), "
		   "'unassigned_message_ids' (массив чисел).";
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
	AssertIsConditionMet(root.contains("unassigned_message_ids") && root["unassigned_message_ids"].is_array(), "Отсутствует массив unassigned_message_ids");

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

	for (const auto& idNode : root["unassigned_message_ids"])
	{
		AssertIsConditionMet(idNode.is_number(), "Идентификатор нераспределенного сообщения должен быть числом");
		split.unassignedMessageIds.push_back(idNode.get<int64_t>());
	}

	return split;
}

struct ValidationResult
{
	bool isValid = false;
	std::vector<int64_t> missingIds;
	std::string errorMessage;
};

ValidationResult CheckCoverage(const SemanticClusterSplit& split, const MessageCluster& cluster)
{
	ValidationResult result;
	result.isValid = true;

	if (split.clusterId != cluster.id)
	{
		result.isValid = false;
		result.errorMessage = "ID кластера в ответе не совпадает с запрошенным";
		return result;
	}

	std::unordered_set<std::string> componentIds;
	std::unordered_set<int64_t> assigned;

	for (const auto& comp : split.components)
	{
		if (componentIds.contains(comp.id))
		{
			result.isValid = false;
			result.errorMessage = "Нейросеть вернула дублирующиеся ID компонентов";
			return result;
		}
		componentIds.insert(comp.id);

		for (const int64_t msgId : comp.messageIds)
		{
			assigned.insert(msgId);
		}
	}

	std::unordered_set<int64_t> unassigned(split.unassignedMessageIds.begin(), split.unassignedMessageIds.end());
	std::unordered_set<int64_t> expected(cluster.messageIds.begin(), cluster.messageIds.end());

	for (const int64_t id : expected)
	{
		if (!assigned.contains(id) && !unassigned.contains(id))
		{
			result.missingIds.push_back(id);
		}
	}

	for (const int64_t id : assigned)
	{
		if (!expected.contains(id))
		{
			result.isValid = false;
			result.errorMessage = "Нейросеть придумала несуществующий ID сообщения (галлюцинация)";
			return result;
		}
		if (unassigned.contains(id))
		{
			result.isValid = false;
			result.errorMessage = "Сообщение находится одновременно в компонентах и в unassigned_message_ids";
			return result;
		}
	}

	for (const int64_t id : unassigned)
	{
		if (!expected.contains(id))
		{
			result.isValid = false;
			result.errorMessage = "Нейросеть добавила несуществующий ID в unassigned_message_ids";
			return result;
		}
	}

	if (!result.missingIds.empty())
	{
		result.isValid = false;
		result.errorMessage = "Нейросеть потеряла часть сообщений при разделении";
	}

	return result;
}

std::string FormatRetryPrompt(const std::string& originalPrompt, const std::vector<int64_t>& missingIds)
{
	std::string retryPrompt = originalPrompt + "\n\nТы пропустил следующие сообщения:\n[";
	for (size_t i = 0; i < missingIds.size(); ++i)
	{
		retryPrompt += std::to_string(missingIds[i]);
		if (i + 1 < missingIds.size())
		{
			retryPrompt += ", ";
		}
	}
	retryPrompt += "].\nРаспредели их по существующим компонентам. Если они действительно не относятся ни к одному, помести их в unassigned_message_ids.";
	return retryPrompt;
}
} // namespace

DeepSeekSemanticSplitter::DeepSeekSemanticSplitter(std::unique_ptr<IAIClient> aiClient)
	: m_aiClient(std::move(aiClient))
{
	AssertIsConditionMet(m_aiClient != nullptr, "Указатель на IAIClient не может быть пустым");
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
		if (cluster.messageIds.size() < 50)
		{
			continue;
		}

		std::string prompt = PrepareAiPrompt(cluster, index);
		SemanticClusterSplit finalSplit;
		bool success = false;

		for (int attempt = 0; attempt < 3; ++attempt)
		{
			const std::string response = m_aiClient->Complete(GetSystemPrompt(), prompt, true);
			SemanticClusterSplit split = ParseAiResponse(response);
			ValidationResult validation = CheckCoverage(split, cluster);

			if (validation.isValid)
			{
				finalSplit = std::move(split);
				success = true;
				break;
			}

			if (attempt == 2)
			{
				AssertIsConditionMet(false, validation.errorMessage);
			}

			if (!validation.missingIds.empty())
			{
				prompt = FormatRetryPrompt(prompt, validation.missingIds);
			}
			else
			{
				AssertIsConditionMet(false, validation.errorMessage);
			}
		}

		if (success)
		{
			results.push_back(std::move(finalSplit));
		}
	}

	return results;
}