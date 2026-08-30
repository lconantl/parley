#pragma once

#include "drawlist/DrawList.hpp"
#include "geometry/Geometry.hpp"
#include "layout/ITextMeasurer.hpp"
#include "theme/Theme.hpp"

#include <string_view>

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
