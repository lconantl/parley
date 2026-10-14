#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Listing
{
	std::string title;
	std::string summary;
	std::string contact;
	std::string date;
	std::string url;
};

struct Match
{
	std::string id;
	int rank = 0;
	int score = 0;
	std::string suggestedMode;
	Listing buyer;
	Listing asset;
	std::vector<std::string> reasons;
	std::vector<std::string> gaps;
	std::string details;
};

struct Draft
{
	std::string label;
	std::string context;
	std::string body;
	bool applicable = true;
};

struct Progress
{
	int total = 0;
	int liked = 0;
	int skipped = 0;
};

struct DecisionEvent
{
	std::int64_t id = 0;
	std::string matchId;
};
