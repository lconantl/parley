#pragma once

#include "../theme/Theme.hpp"
#include "ITextMeasurer.hpp"
#include <hpdf.h>
#include <map>
#include <string_view>

class HaruTextMeasurer : public ITextMeasurer
{
public:
	explicit HaruTextMeasurer(std::map<FontRole, HPDF_Font> fonts);
	~HaruTextMeasurer() override = default;

	double MeasureWidth(std::string_view text, FontRole font, double fontSize) const override;

private:
	std::map<FontRole, HPDF_Font> m_fonts;
};