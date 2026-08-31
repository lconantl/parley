#pragma once

#include <string_view>

struct Color
{
	double red;
	double green;
	double blue;
};

Color ColorFromHex(std::string_view hex);