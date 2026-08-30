#pragma once

#include "ITextMeasurer.hpp"
#include "pdf/graphics/DrawList.hpp"
#include "pdf/theme/Theme.hpp"
#include <string>
#include <string_view>
#include <vector>

enum class TextAlign
{
	Left,
	Center,
	Right
};

struct TextStyle
{
	FontRole font;
	double fontSize;
	double lineHeight;
	Color color;
	TextAlign align;
};

std::vector<std::string> WrapText(
	std::string_view text,
	const TextStyle& style,
	double maxWidth,
	const ITextMeasurer& measurer);

double MeasureTextHeight(
	std::string_view text,
	const TextStyle& style,
	double maxWidth,
	const ITextMeasurer& measurer);

double EmitTextBlock(
	DrawList& target,
	std::string_view text,
	const TextStyle& style,
	const Rect& area,
	const ITextMeasurer& measurer);

void EmitSingleLine(
	DrawList& target,
	std::string_view text,
	const TextStyle& style,
	const Rect& area,
	const ITextMeasurer& measurer);
