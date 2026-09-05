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

struct KpiEntry
{
	std::string label;
	std::string value;
	std::string note;
	bool emphasizeNegative = false;
};

struct KpiSlideContent
{
	std::string title;
	std::vector<KpiEntry> figures;
	std::string commentary;
	std::string sourceNote;
};

struct ChartSeries
{
	std::string name;
	std::vector<double> values;
};

enum class ChartKind
{
	Bars,
	GroupedBars,
	Line
};

struct SecondaryAxisSeries
{
	std::string name;
	std::vector<double> values;
	std::string valueSuffix;
};

struct ChartSlideContent
{
	std::string title;
	ChartKind kind = ChartKind::Bars;
	std::vector<std::string> categories;
	std::vector<ChartSeries> series;
	std::string valueSuffix;
	std::optional<SecondaryAxisSeries> secondarySeries;
	std::string takeaway;
	std::vector<std::string> notes;
	std::string sourceNote;
};

struct TableSlideContent
{
	std::string title;
	std::vector<std::string> headers;
	std::vector<std::vector<std::string>> rows;
	std::string takeaway;
	std::string sourceNote;
};

struct ClosingSlideContent
{
	std::string title;
	std::vector<std::string> lines;
};

struct ValuationBridgeStep
{
	std::string label;
	std::string value;
	std::string annotation;
	bool isTotal = false;
};

struct RevenueMixSegment
{
	std::string name;
	std::string share;
	std::string amount;
	double shareFraction = 0.0;
};

struct OnePagerSlideContent
{
	std::string eyebrow;
	std::string vintageLabel;
	std::string headline;
	std::string subtitle;

	std::vector<KpiEntry> kpis;

	std::vector<ValuationBridgeStep> valuationBridge;
	std::string valuationMultipleNote;

	std::vector<RevenueMixSegment> revenueMix;
	std::vector<NumberedEntry> businessFacts;

	std::optional<ChartSlideContent> trajectory;

	std::vector<NumberedEntry> investmentCase;
	std::vector<std::string> valueCreationSteps;
	std::vector<NumberedEntry> risks;
	std::string nextStep;

	std::string sourceNote;
};

using SlideContent = std::variant<
	TitleSlideContent,
	CardGridSlideContent,
	ColumnStripSlideContent,
	BulletSlideContent,
	KpiSlideContent,
	ChartSlideContent,
	TableSlideContent,
	ClosingSlideContent,
	OnePagerSlideContent>;

struct Slide
{
	SlideContent content;
};