#pragma once

#include "common/output/pdf/model/Color.hpp"
#include "common/output/pdf/model/Geometry.hpp"
#include "common/output/pdf/theme/Theme.hpp"
#include <filesystem>
#include <string>
#include <variant>

struct RectCommand
{
	Rect bounds;
	Color fill;
	double cornerRadius;
};

struct LineCommand
{
	Point from;
	Point to;
	Color stroke;
	double thickness;
};

struct TextCommand
{
	Point lineBoxTopLeft;
	std::string text;
	FontRole font;
	double fontSize;
	double lineHeight;
	Color color;
};

struct ImageCommand
{
	Rect bounds;
	std::filesystem::path source;
};

using DrawCommand = std::variant<RectCommand, LineCommand, TextCommand, ImageCommand>;
