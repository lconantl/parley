#include "HaruTextMeasurer.hpp"
#include <stdexcept>

namespace
{
constexpr double FONT_SCALE_FACTOR = 1000.0;

void AssertIsKnownFont(const std::map<FontRole, HPDF_Font>& fonts, const FontRole role)
{
	if (!fonts.contains(role))
	{
		throw std::invalid_argument("Требуемый шрифт не загружен в измеритель");
	}
}

void AssertIsPositiveFontSize(const double fontSize)
{
	if (fontSize <= 0.0)
	{
		throw std::invalid_argument("Кегль шрифта должен быть положительным");
	}
}

double CalculateScaledWidth(const HPDF_UINT32 rawWidth, const double fontSize)
{
	return static_cast<double>(rawWidth) * fontSize / FONT_SCALE_FACTOR;
}
} // namespace

HaruTextMeasurer::HaruTextMeasurer(std::map<FontRole, HPDF_Font> fonts)
	: m_fonts(std::move(fonts))
{
}

double HaruTextMeasurer::MeasureWidth(const std::string_view text, const FontRole font, const double fontSize) const
{
	AssertIsKnownFont(m_fonts, font);
	AssertIsPositiveFontSize(fontSize);

	if (text.empty())
	{
		return 0.0;
	}

	const HPDF_Font targetFont = m_fonts.at(font);

	const auto* textBytes = reinterpret_cast<const HPDF_BYTE*>(text.data());
	const HPDF_UINT textLength = static_cast<HPDF_UINT>(text.length());

	const HPDF_TextWidth metrics = HPDF_Font_TextWidth(targetFont, textBytes, textLength);

	return CalculateScaledWidth(metrics.width, fontSize);
}