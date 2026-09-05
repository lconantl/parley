#include "SlideChrome.hpp"

#include "TextBlock.hpp"
#include "common/output/pdf/theme/TextStyles.hpp"

#include <stdexcept>
#include <string>

namespace
{
constexpr double LOGO_WIDTH = 34.0;
constexpr double LOGO_HEIGHT = 14.0;
constexpr double PAGE_NUMBER_WIDTH = 18.0;
constexpr double PAGE_NUMBER_GAP = 4.0;

void AssertIsPositivePageNumber(const int pageNumber)
{
	if (pageNumber <= 0)
	{
		throw std::invalid_argument("Номер страницы должен быть положительным");
	}
}

bool HasAsset(const Theme& theme, const AssetRole role)
{
	return theme.assets.contains(role) && !theme.assets.at(role).empty();
}

Rect LogoBounds(const Theme& theme)
{
	const double right = theme.metrics.page.width - theme.metrics.page.marginRight;
	const double bottom = theme.metrics.page.height - theme.metrics.page.marginBottom;

	return Rect{right - LOGO_WIDTH - PAGE_NUMBER_WIDTH, bottom - LOGO_HEIGHT, LOGO_WIDTH, LOGO_HEIGHT};
}

Rect PageNumberBounds(const Theme& theme)
{
	const Rect logo = LogoBounds(theme);

	return Rect{RectRight(logo) + PAGE_NUMBER_GAP, logo.top, PAGE_NUMBER_WIDTH, LOGO_HEIGHT};
}

AssetRole FooterLogoRole(const bool inverse)
{
	return inverse ? AssetRole::LogoInverse : AssetRole::Logo;
}

Color FooterTextColor(const Theme& theme, const bool inverse)
{
	return inverse ? theme.palette.textInverse : theme.palette.text;
}

TextStyle SourceNoteStyle(const Theme& theme)
{
	TextStyle style = CaptionStyle(theme);
	style.color = theme.palette.text;
	style.align = TextAlign::Left;

	return style;
}
} // namespace

Rect SlideBounds(const Theme& theme)
{
	return Rect{0.0, 0.0, theme.metrics.page.width, theme.metrics.page.height};
}

Rect ContentArea(const Theme& theme)
{
	const PageMetrics& page = theme.metrics.page;

	return Rect{
		page.marginLeft,
		theme.metrics.contentTop,
		ContentWidth(theme.metrics),
		page.height - theme.metrics.contentTop - page.marginBottom};
}

void EmitBackground(DrawList& target, const Theme& theme, const Color& fill)
{
	RectCommand command;
	command.bounds = SlideBounds(theme);
	command.fill = fill;
	command.cornerRadius = 0.0;

	target.AddRect(command);
}

void EmitDecoration(DrawList& target, const Theme& theme, const AssetRole role, const Rect& bounds)
{
	if (!HasAsset(theme, role))
	{
		return;
	}

	ImageCommand command;
	command.bounds = bounds;
	command.source = theme.assets.at(role);

	target.AddImage(command);
}

void EmitFooter(DrawList& target, const Theme& theme, const int pageNumber, const bool inverse, const ITextMeasurer& measurer)
{
	AssertIsPositivePageNumber(pageNumber);

	EmitDecoration(target, theme, FooterLogoRole(inverse), LogoBounds(theme));

	TextStyle style = CaptionStyle(theme);
	style.color = FooterTextColor(theme, inverse);
	style.align = TextAlign::Left;

	EmitSingleLine(target, std::to_string(pageNumber), style, PageNumberBounds(theme), measurer);
}

double EmitSlideTitle(
	DrawList& target,
	const Theme& theme,
	const std::string_view title,
	const double maxWidth,
	const bool inverse,
	const ITextMeasurer& measurer)
{
	TextStyle style = SlideTitleStyle(theme);
	style.color = inverse ? theme.palette.textInverse : theme.palette.text;

	const auto area = Rect{
		theme.metrics.page.marginLeft,
		theme.metrics.titleBlockTop,
		maxWidth,
		theme.metrics.titleBlockHeight};

	return EmitTextBlock(target, title, style, area, measurer);
}

void EmitSourceNote(
	DrawList& target,
	const std::string& note,
	const Theme& theme,
	const ITextMeasurer& measurer)
{
	if (note.empty())
	{
		return;
	}

	const TextStyle style = SourceNoteStyle(theme);

	const Rect area = Rect{
		theme.metrics.page.marginLeft,
		theme.metrics.page.height - SOURCE_NOTE_BOTTOM,
		ContentWidth(theme.metrics),
		SOURCE_NOTE_HEIGHT};

	EmitSingleLine(target, note, style, area, measurer);
}

void EmitTakeaway(
	DrawList& target,
	const std::string& takeaway,
	const Theme& theme,
	const Rect& area,
	const ITextMeasurer& measurer)
{
	if (takeaway.empty())
	{
		return;
	}

	RectCommand panel;
	panel.bounds = area;
	panel.fill = theme.palette.ink;
	panel.cornerRadius = theme.metrics.card.cornerRadius;

	target.AddRect(panel);

	TextStyle style = BodyInverseStyle(theme);
	style.align = TextAlign::Left;

	const Rect textArea = InsetRect(area, theme.metrics.card.padding);

	EmitTextBlock(target, takeaway, style, textArea, measurer);
}