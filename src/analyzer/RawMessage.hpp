#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

struct TextEntity
{
	std::string type;
	std::string text;
	std::optional<std::string> href;
	std::optional<std::string> documentId;
};

struct ContactInformation
{
	std::optional<std::string> firstName;
	std::optional<std::string> lastName;
	std::optional<std::string> phoneNumber;
};

struct PollAnswer
{
	std::string text;
	int voters = 0;
	bool chosen = false;
};

struct Poll
{
	std::string question;
	bool closed = false;
	int totalVoters = 0;
	std::vector<PollAnswer> answers;
};

struct ReactionRecent
{
	std::string from;
	std::string fromId;
	std::string date;
};

struct Reaction
{
	std::string type;
	int count = 0;
	std::optional<std::string> emoji;
	std::vector<ReactionRecent> recent;
};

struct RawMessage
{
	std::optional<int64_t> id;
	std::optional<std::string> type;
	std::optional<std::string> date;
	std::optional<std::string> dateUnixtime;

	std::optional<std::string> from;
	std::optional<std::string> fromId;
	std::optional<std::string> actor;
	std::optional<std::string> actorId;
	std::optional<std::string> action;

	std::optional<std::string> forwardedFrom;
	std::optional<std::string> forwardedFromId;

	std::optional<int64_t> replyToMessageId;
	std::optional<std::string> replyToPeerId;

	std::optional<std::string> edited;
	std::optional<std::string> editedUnixtime;

	nlohmann::json text;
	std::vector<TextEntity> textEntities;

	std::optional<std::string> photo;
	std::optional<int64_t> photoFileSize;
	std::optional<int64_t> width;
	std::optional<int64_t> height;

	std::optional<std::string> file;
	std::optional<std::string> fileName;
	std::optional<int64_t> fileSize;
	std::optional<std::string> mimeType;
	std::optional<std::string> thumbnail;
	std::optional<int64_t> thumbnailFileSize;

	std::optional<std::string> mediaType;
	std::optional<int64_t> durationSeconds;

	std::optional<ContactInformation> contactInformation;
	std::optional<std::string> contactVcard;
	std::optional<int64_t> contactVcardFileSize;

	std::optional<Poll> poll;
	std::vector<Reaction> reactions;

	std::vector<std::string> members;
	std::optional<std::string> title;
	std::optional<std::string> newTitle;
	std::optional<std::string> newIconEmojiId;

	std::optional<std::string> stickerEmoji;
	std::optional<bool> mediaSpoiler;
	std::optional<bool> richMessage;
	std::optional<std::string> viaBot;
	nlohmann::json inlineBotButtons;

	nlohmann::json extraFields = nlohmann::json::object();
	nlohmann::json rawJson = nlohmann::json::object();
};