#pragma once

#include "ITextMeasurer.hpp"
#include "common/output/pdf/graphics/DrawList.hpp"
#include "common/output/pdf/model/Geometry.hpp"
#include "common/output/pdf/theme/Theme.hpp"
#include <string>
#include <string_view>

constexpr double SOURCE_NOTE_BOTTOM = 44.0;
constexpr double SOURCE_NOTE_HEIGHT = 16.0;

Rect SlideBounds(const Theme& theme);
Rect ContentArea(const Theme& theme);

void EmitBackground(DrawList& target, const Theme& theme, const Color& fill);
void EmitDecoration(DrawList& target, const Theme& theme, AssetRole role, const Rect& bounds);
void EmitFooter(DrawList& target, const Theme& theme, int pageNumber, bool inverse, const ITextMeasurer& measurer);

double EmitSlideTitle(
	DrawList& target,
	const Theme& theme,
	std::string_view title,
	double maxWidth,
	bool inverse,
	const ITextMeasurer& measurer);

void EmitSourceNote(
	DrawList& target,
	const std::string& note,
	const Theme& theme,
	const ITextMeasurer& measurer);

void EmitTakeaway(
	DrawList& target,
	const std::string& takeaway,
	const Theme& theme,
	const Rect& area,
	const ITextMeasurer& measurer);
