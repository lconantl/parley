#include "TelegramExportParser.hpp"
#include <charconv>
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

void AssertIsMessagesArrayExists(const bool exists)
{
	if (!exists)
	{
		throw std::runtime_error("Массив 'messages' не найден в корневом объекте JSON");
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

template <typename T>
std::optional<T> ExtractValue(const nlohmann::json& node, std::string_view key)
{
	if (!node.contains(key) || node[key].is_null())
	{
		return std::nullopt;
	}

	if constexpr (std::is_same_v<T, std::string>)
	{
		if (node[key].is_string())
		{
			return node[key].get<std::string>();
		}
		if (node[key].is_number())
		{
			return std::to_string(node[key].get<int64_t>());
		}
	}
	else if constexpr (std::is_same_v<T, int64_t>)
	{
		if (node[key].is_number())
		{
			return node[key].get<int64_t>();
		}
		if (node[key].is_string())
		{
			int64_t result = 0;
			const auto& str = node[key].get_ref<const std::string&>();
			if (auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result); ec == std::errc{})
			{
				return result;
			}
		}
	}
	else if constexpr (std::is_same_v<T, bool>)
	{
		if (node[key].is_boolean())
		{
			return node[key].get<bool>();
		}
	}

	return std::nullopt;
}

const std::unordered_set<std::string>& GetKnownFields()
{
	static const std::unordered_set<std::string> fields = {
		"id", "type", "date", "date_unixtime", "from", "from_id", "actor", "actor_id", "action", "forwarded_from", "forwarded_from_id", "reply_to_message_id", "reply_to_peer_id", "edited", "edited_unixtime", "text", "text_entities", "photo", "photo_file_size", "width", "height", "file", "file_name", "file_size", "mime_type", "thumbnail", "thumbnail_file_size", "media_type", "duration_seconds", "contact_information", "contact_vcard", "contact_vcard_file_size", "poll", "reactions", "members", "title", "inline_bot_buttons", "via_bot", "sticker_emoji", "media_spoiler", "rich_message", "new_title", "new_icon_emoji_id"};
	return fields;
}

void ParseBaseMetadata(const nlohmann::json& node, RawMessage& message)
{
	message.id = ExtractValue<int64_t>(node, "id");
	message.type = ExtractValue<std::string>(node, "type");
	message.date = ExtractValue<std::string>(node, "date");
	message.dateUnixtime = ExtractValue<std::string>(node, "date_unixtime");
	message.edited = ExtractValue<std::string>(node, "edited");
	message.editedUnixtime = ExtractValue<std::string>(node, "edited_unixtime");
}

void ParseSenderMetadata(const nlohmann::json& node, RawMessage& message)
{
	message.from = ExtractValue<std::string>(node, "from");
	message.fromId = ExtractValue<std::string>(node, "from_id");
	message.actor = ExtractValue<std::string>(node, "actor");
	message.actorId = ExtractValue<std::string>(node, "actor_id");
	message.action = ExtractValue<std::string>(node, "action");
	message.forwardedFrom = ExtractValue<std::string>(node, "forwarded_from");
	message.forwardedFromId = ExtractValue<std::string>(node, "forwarded_from_id");
}

void ParseRelationsMetadata(const nlohmann::json& node, RawMessage& message)
{
	message.replyToMessageId = ExtractValue<int64_t>(node, "reply_to_message_id");
	message.replyToPeerId = ExtractValue<std::string>(node, "reply_to_peer_id");
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
		entity.type = ExtractValue<std::string>(entityNode, "type").value_or("");
		entity.text = ExtractValue<std::string>(entityNode, "text").value_or("");
		entity.href = ExtractValue<std::string>(entityNode, "href");
		entity.documentId = ExtractValue<std::string>(entityNode, "document_id");
		message.textEntities.push_back(std::move(entity));
	}
}

void ParseMediaMetadata(const nlohmann::json& node, RawMessage& message)
{
	message.photo = ExtractValue<std::string>(node, "photo");
	message.photoFileSize = ExtractValue<int64_t>(node, "photo_file_size");
	message.width = ExtractValue<int64_t>(node, "width");
	message.height = ExtractValue<int64_t>(node, "height");
	message.file = ExtractValue<std::string>(node, "file");
	message.fileName = ExtractValue<std::string>(node, "file_name");
	message.fileSize = ExtractValue<int64_t>(node, "file_size");
	message.mimeType = ExtractValue<std::string>(node, "mime_type");
	message.thumbnail = ExtractValue<std::string>(node, "thumbnail");
	message.thumbnailFileSize = ExtractValue<int64_t>(node, "thumbnail_file_size");
	message.mediaType = ExtractValue<std::string>(node, "media_type");
	message.durationSeconds = ExtractValue<int64_t>(node, "duration_seconds");
	message.mediaSpoiler = ExtractValue<bool>(node, "media_spoiler");
}

void ParseContactMetadata(const nlohmann::json& node, RawMessage& message)
{
	message.contactVcard = ExtractValue<std::string>(node, "contact_vcard");
	message.contactVcardFileSize = ExtractValue<int64_t>(node, "contact_vcard_file_size");

	if (node.contains("contact_information") && node["contact_information"].is_object())
	{
		const auto& infoNode = node["contact_information"];
		ContactInformation info;
		info.firstName = ExtractValue<std::string>(infoNode, "first_name");
		info.lastName = ExtractValue<std::string>(infoNode, "last_name");
		info.phoneNumber = ExtractValue<std::string>(infoNode, "phone_number");
		message.contactInformation = std::move(info);
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
	poll.question = ExtractValue<std::string>(pollNode, "question").value_or("");
	poll.closed = ExtractValue<bool>(pollNode, "closed").value_or(false);
	poll.totalVoters = static_cast<int>(ExtractValue<int64_t>(pollNode, "total_voters").value_or(0));

	if (pollNode.contains("answers") && pollNode["answers"].is_array())
	{
		for (const auto& answerNode : pollNode["answers"])
		{
			PollAnswer answer;
			answer.text = ExtractValue<std::string>(answerNode, "text").value_or("");
			answer.voters = static_cast<int>(ExtractValue<int64_t>(answerNode, "voters").value_or(0));
			answer.chosen = ExtractValue<bool>(answerNode, "chosen").value_or(false);
			poll.answers.push_back(std::move(answer));
		}
	}
	message.poll = std::move(poll);
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
		reaction.type = ExtractValue<std::string>(reactionNode, "type").value_or("");
		reaction.count = static_cast<int>(ExtractValue<int64_t>(reactionNode, "count").value_or(0));
		reaction.emoji = ExtractValue<std::string>(reactionNode, "emoji");

		if (reactionNode.contains("recent") && reactionNode["recent"].is_array())
		{
			for (const auto& recentNode : reactionNode["recent"])
			{
				ReactionRecent recent;
				recent.from = ExtractValue<std::string>(recentNode, "from").value_or("");
				recent.fromId = ExtractValue<std::string>(recentNode, "from_id").value_or("");
				recent.date = ExtractValue<std::string>(recentNode, "date").value_or("");
				reaction.recent.push_back(std::move(recent));
			}
		}
		message.reactions.push_back(std::move(reaction));
	}
}

void ParseServiceMetadata(const nlohmann::json& node, RawMessage& message)
{
	message.title = ExtractValue<std::string>(node, "title");
	message.newTitle = ExtractValue<std::string>(node, "new_title");
	message.newIconEmojiId = ExtractValue<std::string>(node, "new_icon_emoji_id");
	message.stickerEmoji = ExtractValue<std::string>(node, "sticker_emoji");
	message.richMessage = ExtractValue<bool>(node, "rich_message");
	message.viaBot = ExtractValue<std::string>(node, "via_bot");

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

	if (node.contains("text"))
	{
		message.text = node["text"];
	}

	ParseBaseMetadata(node, message);
	ParseSenderMetadata(node, message);
	ParseRelationsMetadata(node, message);
	ParseTextEntities(node, message);
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

	const auto& messagesArray = rootNode["messages"];
	std::vector<RawMessage> parsedMessages;
	parsedMessages.reserve(messagesArray.size());

	for (const auto& messageNode : messagesArray)
	{
		parsedMessages.push_back(ParseSingleMessage(messageNode));
	}

	return parsedMessages;
}