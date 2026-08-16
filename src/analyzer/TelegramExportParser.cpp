#include "TelegramExportParser.hpp"
#include <fstream>
#include <stdexcept>
#include <unordered_set>

namespace
{
void AssertIsFileExists(const bool exists)
{
	if (!exists)
	{
		throw std::runtime_error("Указанный файл экспорта JSON не найден");
	}
}

void AssertIsFileOpen(const bool isOpen)
{
	if (!isOpen)
	{
		throw std::runtime_error("Не удалось открыть файл экспорта JSON для чтения");
	}
}

nlohmann::json ReadJsonFromFile(const std::filesystem::path& path)
{
	std::ifstream file(path);
	AssertIsFileOpen(file.is_open());

	try
	{
		return nlohmann::json::parse(file);
	}
	catch (const nlohmann::json::parse_error&)
	{
		throw std::runtime_error("Ошибка парсинга файла: невалидный формат JSON");
	}
}

void AssertIsMessagesArrayExists(const bool exists)
{
	if (!exists)
	{
		throw std::runtime_error("Массив 'messages' не найден в корневом объекте JSON");
	}
}

const std::unordered_set<std::string>& GetKnownFields()
{
	static const std::unordered_set<std::string> fields = {
		"id", "type", "date", "date_unixtime", "from", "from_id", "actor", "actor_id", "action", "forwarded_from", "forwarded_from_id", "reply_to_message_id", "reply_to_peer_id", "edited", "edited_unixtime", "text", "text_entities", "photo", "photo_file_size", "width", "height", "file", "file_name", "file_size", "mime_type", "thumbnail", "thumbnail_file_size", "media_type", "duration_seconds", "contact_information", "contact_vcard", "contact_vcard_file_size", "poll", "reactions", "members", "title", "inline_bot_buttons", "via_bot", "sticker_emoji", "media_spoiler", "rich_message", "new_title", "new_icon_emoji_id"};
	return fields;
}

std::optional<std::string> ExtractString(const nlohmann::json& node, const std::string& key)
{
	if (node.contains(key) && node[key].is_string())
	{
		return node[key].get<std::string>();
	}
	return std::nullopt;
}

std::optional<int64_t> ExtractInt64(const nlohmann::json& node, const std::string& key)
{
	if (node.contains(key) && node[key].is_number())
	{
		return node[key].get<int64_t>();
	}
	if (node.contains(key) && node[key].is_string())
	{
		try
		{
			return std::stoll(node[key].get<std::string>());
		}
		catch (...)
		{
			return std::nullopt;
		}
	}
	return std::nullopt;
}

std::optional<bool> ExtractBool(const nlohmann::json& node, const std::string& key)
{
	if (node.contains(key) && node[key].is_boolean())
	{
		return node[key].get<bool>();
	}
	return std::nullopt;
}

void ParseBaseMetadata(const nlohmann::json& node, RawMessage& message)
{
	message.id = ExtractInt64(node, "id");
	message.type = ExtractString(node, "type");
	message.date = ExtractString(node, "date");
	message.dateUnixtime = ExtractString(node, "date_unixtime");
	message.edited = ExtractString(node, "edited");
	message.editedUnixtime = ExtractString(node, "edited_unixtime");
}

void ParseSenderMetadata(const nlohmann::json& node, RawMessage& message)
{
	message.from = ExtractString(node, "from");
	message.fromId = ExtractString(node, "from_id");
	message.actor = ExtractString(node, "actor");
	message.actorId = ExtractString(node, "actor_id");
	message.action = ExtractString(node, "action");
	message.forwardedFrom = ExtractString(node, "forwarded_from");
	message.forwardedFromId = ExtractString(node, "forwarded_from_id");
}

void ParseRelationsMetadata(const nlohmann::json& node, RawMessage& message)
{
	message.replyToMessageId = ExtractInt64(node, "reply_to_message_id");
	message.replyToPeerId = ExtractString(node, "reply_to_peer_id");
}

void ParseTextEntities(const nlohmann::json& node, RawMessage& message)
{
	if (!node.contains("text_entities") || !node["text_entities"].is_array())
	{
		return;
	}

	for (const auto& entityNode : node["text_entities"])
	{
		TextEntity entity;
		entity.type = ExtractString(entityNode, "type").value_or("");
		entity.text = ExtractString(entityNode, "text").value_or("");
		entity.href = ExtractString(entityNode, "href");
		entity.documentId = ExtractString(entityNode, "document_id");
		message.textEntities.push_back(entity);
	}
}

void ParseTextMetadata(const nlohmann::json& node, RawMessage& message)
{
	if (node.contains("text"))
	{
		message.text = node["text"];
	}
	ParseTextEntities(node, message);
}

void ParseMediaMetadata(const nlohmann::json& node, RawMessage& message)
{
	message.photo = ExtractString(node, "photo");
	message.photoFileSize = ExtractInt64(node, "photo_file_size");
	message.width = ExtractInt64(node, "width");
	message.height = ExtractInt64(node, "height");

	message.file = ExtractString(node, "file");
	message.fileName = ExtractString(node, "file_name");
	message.fileSize = ExtractInt64(node, "file_size");
	message.mimeType = ExtractString(node, "mime_type");
	message.thumbnail = ExtractString(node, "thumbnail");
	message.thumbnailFileSize = ExtractInt64(node, "thumbnail_file_size");

	message.mediaType = ExtractString(node, "media_type");
	message.durationSeconds = ExtractInt64(node, "duration_seconds");
	message.mediaSpoiler = ExtractBool(node, "media_spoiler");
}

void ParseContactMetadata(const nlohmann::json& node, RawMessage& message)
{
	message.contactVcard = ExtractString(node, "contact_vcard");
	message.contactVcardFileSize = ExtractInt64(node, "contact_vcard_file_size");

	if (node.contains("contact_information") && node["contact_information"].is_object())
	{
		const auto& infoNode = node["contact_information"];
		ContactInformation info;
		info.firstName = ExtractString(infoNode, "first_name");
		info.lastName = ExtractString(infoNode, "last_name");
		info.phoneNumber = ExtractString(infoNode, "phone_number");
		message.contactInformation = info;
	}
}

void ParsePollMetadata(const nlohmann::json& node, RawMessage& message)
{
	if (!node.contains("poll") || !node["poll"].is_object())
	{
		return;
	}

	const auto& pollNode = node["poll"];
	Poll poll;
	poll.question = ExtractString(pollNode, "question").value_or("");
	poll.closed = ExtractBool(pollNode, "closed").value_or(false);
	poll.totalVoters = static_cast<int>(ExtractInt64(pollNode, "total_voters").value_or(0));

	if (pollNode.contains("answers") && pollNode["answers"].is_array())
	{
		for (const auto& answerNode : pollNode["answers"])
		{
			PollAnswer answer;
			answer.text = ExtractString(answerNode, "text").value_or("");
			answer.voters = static_cast<int>(ExtractInt64(answerNode, "voters").value_or(0));
			answer.chosen = ExtractBool(answerNode, "chosen").value_or(false);
			poll.answers.push_back(answer);
		}
	}
	message.poll = poll;
}

void ParseReactionsMetadata(const nlohmann::json& node, RawMessage& message)
{
	if (!node.contains("reactions") || !node["reactions"].is_array())
	{
		return;
	}

	for (const auto& reactionNode : node["reactions"])
	{
		Reaction reaction;
		reaction.type = ExtractString(reactionNode, "type").value_or("");
		reaction.count = static_cast<int>(ExtractInt64(reactionNode, "count").value_or(0));
		reaction.emoji = ExtractString(reactionNode, "emoji");

		if (reactionNode.contains("recent") && reactionNode["recent"].is_array())
		{
			for (const auto& recentNode : reactionNode["recent"])
			{
				ReactionRecent recent;
				recent.from = ExtractString(recentNode, "from").value_or("");
				recent.fromId = ExtractString(recentNode, "from_id").value_or("");
				recent.date = ExtractString(recentNode, "date").value_or("");
				reaction.recent.push_back(recent);
			}
		}
		message.reactions.push_back(reaction);
	}
}

void ParseServiceMetadata(const nlohmann::json& node, RawMessage& message)
{
	message.title = ExtractString(node, "title");
	message.newTitle = ExtractString(node, "new_title");
	message.newIconEmojiId = ExtractString(node, "new_icon_emoji_id");
	message.stickerEmoji = ExtractString(node, "sticker_emoji");
	message.richMessage = ExtractBool(node, "rich_message");
	message.viaBot = ExtractString(node, "via_bot");

	if (node.contains("inline_bot_buttons"))
	{
		message.inlineBotButtons = node["inline_bot_buttons"];
	}

	if (node.contains("members") && node["members"].is_array())
	{
		for (const auto& memberNode : node["members"])
		{
			if (memberNode.is_string())
			{
				message.members.push_back(memberNode.get<std::string>());
			}
		}
	}
}

void ExtractExtraFields(const nlohmann::json& node, RawMessage& message)
{
	const auto& knownFields = GetKnownFields();
	for (auto iterator = node.begin(); iterator != node.end(); ++iterator)
	{
		if (!knownFields.contains(iterator.key()))
		{
			message.extraFields[iterator.key()] = iterator.value();
		}
	}
}

RawMessage ParseSingleMessage(const nlohmann::json& node)
{
	RawMessage message;
	message.rawJson = node;

	ParseBaseMetadata(node, message);
	ParseSenderMetadata(node, message);
	ParseRelationsMetadata(node, message);
	ParseTextMetadata(node, message);
	ParseMediaMetadata(node, message);
	ParseContactMetadata(node, message);
	ParsePollMetadata(node, message);
	ParseReactionsMetadata(node, message);
	ParseServiceMetadata(node, message);
	ExtractExtraFields(node, message);

	return message;
}
} // namespace

TelegramExportParser::TelegramExportParser()
{
}

TelegramExportParser::~TelegramExportParser()
{
}

std::vector<RawMessage> TelegramExportParser::Parse(const std::filesystem::path& path) const
{
	AssertIsFileExists(std::filesystem::exists(path));

	const nlohmann::json rootNode = ReadJsonFromFile(path);
	AssertIsMessagesArrayExists(rootNode.contains("messages") && rootNode["messages"].is_array());

	std::vector<RawMessage> parsedMessages;
	parsedMessages.reserve(rootNode["messages"].size());

	for (const auto& messageNode : rootNode["messages"])
	{
		parsedMessages.push_back(ParseSingleMessage(messageNode));
	}

	return parsedMessages;
}