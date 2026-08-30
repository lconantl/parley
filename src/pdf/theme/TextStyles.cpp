#include "TextStyles.hpp"

namespace
{
double LineHeight(const Theme& theme, const double fontSize)
{
	return fontSize * theme.type.lineHeightFactor;
}

TextStyle MakeStyle(const Theme& theme, const FontRole font, const double fontSize, const Color& color, const TextAlign align)
{
	TextStyle style;
	style.font = font;
	style.fontSize = fontSize;
	style.lineHeight = LineHeight(theme, fontSize);
	style.color = color;
	style.align = align;

	return style;
}
} // namespace

TextStyle SlideTitleStyle(const Theme& theme)
{
	return MakeStyle(theme, FontRole::Regular, theme.type.slideTitle, theme.palette.text, TextAlign::Left);
}

TextStyle CardTitleStyle(const Theme& theme)
{
	return MakeStyle(theme, FontRole::Bold, theme.type.cardTitle, theme.palette.text, TextAlign::Left);
}

TextStyle BodyStyle(const Theme& theme)
{
	return MakeStyle(theme, FontRole::Regular, theme.type.body, theme.palette.text, TextAlign::Left);
}

TextStyle BodyInverseStyle(const Theme& theme)
{
	return MakeStyle(theme, FontRole::Regular, theme.type.body, theme.palette.textInverse, TextAlign::Left);
}

TextStyle CaptionStyle(const Theme& theme)
{
	return MakeStyle(theme, FontRole::Regular, theme.type.caption, theme.palette.textInverse, TextAlign::Left);
}

TextStyle IndexNumberStyle(const Theme& theme)
{
	return MakeStyle(theme, FontRole::Light, theme.type.indexNumber, theme.palette.accent, TextAlign::Right);
}

TextStyle HeroTitleStyle(const Theme& theme)
{
	return MakeStyle(theme, FontRole::Regular, theme.type.heroNumber, theme.palette.text, TextAlign::Left);
}
