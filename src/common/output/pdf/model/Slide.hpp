#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct NumberedEntry
{
	std::string title;
	std::string body;
};

struct TitleSlideContent
{
	std::string title;
	std::string caption;
};

struct CardGridSlideContent
{
	std::string title;
	std::vector<NumberedEntry> cards;
	int columnCount;
};

struct ColumnStripSlideContent
{
	std::string title;
	std::vector<NumberedEntry> columns;
};

struct BulletSlideContent
{
	std::string title;
	std::vector<std::string> bullets;
	std::optional<std::filesystem::path> illustration;
};

struct ClosingSlideContent
{
	std::string title;
	std::vector<std::string> lines;
};

using SlideContent = std::variant<
	TitleSlideContent,
	CardGridSlideContent,
	ColumnStripSlideContent,
	BulletSlideContent,
	ClosingSlideContent>;

struct Slide
{
	SlideContent content;
};