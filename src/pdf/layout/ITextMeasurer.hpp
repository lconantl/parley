#pragma once

#include "theme/Theme.hpp"

#include <string_view>

class ITextMeasurer
{
public:
    virtual ~ITextMeasurer() = default;

    virtual double MeasureWidth(std::string_view text, FontRole font, double fontSize) const = 0;
};
