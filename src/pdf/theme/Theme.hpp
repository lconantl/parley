#pragma once

#include "../color/Color.hpp"
#include <filesystem>
#include <map>
#include <vector>

enum class FontRole
{
	Regular,
	Bold,
	Light
};

enum class AssetRole
{
	Logo,
	LogoInverse,
	TitleDecoration,
	SideDecoration,
	FooterDecoration
};

struct Palette
{
	Color ink;
	Color text;
	Color textInverse;
	Color surface;
	Color background;
	Color accent;
	Color accentSoft;
	Color link;
	Color violet;
};

struct GradientRamp
{
	std::vector<Color> stops;
};

struct TypeScale
{
	double slideTitle;
	double cardTitle;
	double body;
	double caption;
	double indexNumber;
	double heroNumber;
	double lineHeightFactor;
	double paragraphSpacing;
};

struct PageMetrics
{
	double width;
	double height;
	double marginLeft;
	double marginRight;
	double marginTop;
	double marginBottom;
};

struct CardMetrics
{
	double cornerRadius;
	double padding;
	double columnGap;
	double rowGap;
	double titleToBodyGap;
};

struct BandMetrics
{
	double height;
	double topOffset;
};

struct Metrics
{
	PageMetrics page;
	CardMetrics card;
	BandMetrics band;
	double titleBlockTop;
	double titleBlockHeight;
	double contentTop;
};

struct Theme
{
	Palette palette;
	GradientRamp gradient;
	TypeScale type;
	Metrics metrics;
	std::map<FontRole, std::filesystem::path> fonts;
	std::map<AssetRole, std::filesystem::path> assets;
};

double ContentWidth(const Metrics& metrics);
double ColumnWidth(const Metrics& metrics, int columnCount);
double ColumnOffset(const Metrics& metrics, int columnCount, int columnIndex);