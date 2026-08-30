#include "SlideLayoutEngine.hpp"
#include "SlideChrome.hpp"
#include "pdf/model/Slide.hpp"
#include "pdf/theme/TextStyles.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
constexpr double INDEX_COLUMN_WIDTH = 48.0;
constexpr double INDEX_TOP_OFFSET = 2.0;
constexpr double SEPARATOR_THICKNESS = 0.75;
constexpr double TITLE_TO_CONTENT_GAP = 22.0;
constexpr double BULLET_MARKER_SIZE = 5.0;
constexpr double BULLET_MARKER_GAP = 12.0;
constexpr double HERO_TITLE_TOP = 172.8;
constexpr double HERO_BAND_HEIGHT = 102.0;
constexpr double HERO_CAPTION_OFFSET = 38.0;
constexpr double SIDE_DECORATION_WIDTH = 230.0;
constexpr int DEFAULT_COLUMN_COUNT = 3;

template <typename... Handlers>
struct Overloaded : Handlers...
{
	using Handlers::operator()...;
};

void AssertIsNotEmpty(const std::vector<NumberedEntry>& entries)
{
	if (entries.empty())
	{
		throw std::invalid_argument("Слайд должен содержать хотя бы один блок");
	}
}

void AssertIsUsableColumnCount(int columnCount)
{
	if (columnCount <= 0 || columnCount > 6)
	{
		throw std::invalid_argument("Количество колонок должно быть в диапазоне от одной до шести");
	}
}

int ResolveColumnCount(int requested)
{
	return requested > 0 ? requested : DEFAULT_COLUMN_COUNT;
}

std::string FormatIndex(std::size_t position)
{
	const std::size_t humanPosition = position + 1;
	const std::string digits = std::to_string(humanPosition);

	return humanPosition < 10 ? "0" + digits : digits;
}

Rect IndexArea(const Rect& bounds, const Theme& theme)
{
	return Rect{
		RectRight(bounds) - theme.metrics.card.padding - INDEX_COLUMN_WIDTH,
		bounds.top + theme.metrics.card.padding - INDEX_TOP_OFFSET,
		INDEX_COLUMN_WIDTH,
		theme.type.indexNumber};
}

double EntryTitleWidth(double containerWidth, const Theme& theme)
{
	return containerWidth - theme.metrics.card.padding * 2.0 - INDEX_COLUMN_WIDTH;
}

double EntryBodyWidth(double containerWidth, const Theme& theme)
{
	return containerWidth - theme.metrics.card.padding * 2.0;
}

double MeasureEntryHeight(
	const NumberedEntry& entry,
	const Theme& theme,
	double containerWidth,
	const ITextMeasurer& measurer)
{
	const double titleHeight = MeasureTextHeight(
		entry.title, CardTitleStyle(theme), EntryTitleWidth(containerWidth, theme), measurer);

	const double bodyHeight = MeasureTextHeight(
		entry.body, BodyStyle(theme), EntryBodyWidth(containerWidth, theme), measurer);

	return theme.metrics.card.padding * 2.0
		+ titleHeight
		+ theme.metrics.card.titleToBodyGap
		+ bodyHeight;
}

double MeasureRowHeight(
	const std::vector<NumberedEntry>& entries,
	std::size_t rowStart,
	std::size_t rowSize,
	const Theme& theme,
	double containerWidth,
	const ITextMeasurer& measurer)
{
	double tallest = 0.0;
	const std::size_t rowEnd = std::min(rowStart + rowSize, entries.size());

	for (std::size_t index = rowStart; index < rowEnd; ++index)
	{
		tallest = std::max(tallest, MeasureEntryHeight(entries[index], theme, containerWidth, measurer));
	}

	return tallest;
}

void EmitEntryText(
	DrawList& target,
	const NumberedEntry& entry,
	std::size_t position,
	const Theme& theme,
	const Rect& bounds,
	const ITextMeasurer& measurer)
{
	EmitSingleLine(target, FormatIndex(position), IndexNumberStyle(theme), IndexArea(bounds, theme), measurer);

	const Rect titleArea = Rect{
		bounds.left + theme.metrics.card.padding,
		bounds.top + theme.metrics.card.padding,
		EntryTitleWidth(bounds.width, theme),
		bounds.height};

	const double titleHeight = EmitTextBlock(target, entry.title, CardTitleStyle(theme), titleArea, measurer);

	const Rect bodyArea = Rect{
		bounds.left + theme.metrics.card.padding,
		titleArea.top + titleHeight + theme.metrics.card.titleToBodyGap,
		EntryBodyWidth(bounds.width, theme),
		bounds.height};

	EmitTextBlock(target, entry.body, BodyStyle(theme), bodyArea, measurer);
}

void EmitCard(
	DrawList& target,
	const NumberedEntry& entry,
	std::size_t position,
	const Theme& theme,
	const Rect& bounds,
	const ITextMeasurer& measurer)
{
	RectCommand background;
	background.bounds = bounds;
	background.fill = theme.palette.surface;
	background.cornerRadius = theme.metrics.card.cornerRadius;

	target.AddRect(background);
	EmitEntryText(target, entry, position, theme, bounds, measurer);
}

void EmitCardRow(
	DrawList& target,
	const std::vector<NumberedEntry>& entries,
	std::size_t rowStart,
	int columnCount,
	const Theme& theme,
	const Rect& rowBounds,
	const ITextMeasurer& measurer)
{
	const double cardWidth = ColumnWidth(theme.metrics, columnCount);
	const std::size_t rowEnd = std::min(rowStart + static_cast<std::size_t>(columnCount), entries.size());

	for (std::size_t index = rowStart; index < rowEnd; ++index)
	{
		const int column = static_cast<int>(index - rowStart);

		const Rect cardBounds = Rect{
			ColumnOffset(theme.metrics, columnCount, column),
			rowBounds.top,
			cardWidth,
			rowBounds.height};

		EmitCard(target, entries[index], index, theme, cardBounds, measurer);
	}
}

void EmitCardGrid(
	DrawList& target,
	const CardGridSlideContent& content,
	const Theme& theme,
	const Rect& area,
	const ITextMeasurer& measurer)
{
	const int columnCount = ResolveColumnCount(content.columnCount);
	AssertIsUsableColumnCount(columnCount);

	const double cardWidth = ColumnWidth(theme.metrics, columnCount);
	const std::size_t rowSize = static_cast<std::size_t>(columnCount);
	double cursor = area.top;

	for (std::size_t rowStart = 0; rowStart < content.cards.size(); rowStart += rowSize)
	{
		const double rowHeight = MeasureRowHeight(content.cards, rowStart, rowSize, theme, cardWidth, measurer);

		const Rect rowBounds = Rect{area.left, cursor, area.width, rowHeight};

		EmitCardRow(target, content.cards, rowStart, columnCount, theme, rowBounds, measurer);
		cursor += rowHeight + theme.metrics.card.rowGap;
	}
}

void EmitStripSeparators(
	DrawList& target,
	std::size_t columnCount,
	const Theme& theme,
	const Rect& area)
{
	for (std::size_t index = 1; index < columnCount; ++index)
	{
		const double x = ColumnOffset(theme.metrics, static_cast<int>(columnCount), static_cast<int>(index))
			- theme.metrics.card.columnGap / 2.0;

		LineCommand separator;
		separator.from = Point{x, area.top};
		separator.to = Point{x, RectBottom(area)};
		separator.stroke = theme.palette.text;
		separator.thickness = SEPARATOR_THICKNESS;

		target.AddLine(separator);
	}
}

void EmitIndexBand(
	DrawList& target,
	std::size_t columnCount,
	const Theme& theme,
	const Rect& bandBounds,
	const ITextMeasurer& measurer)
{
	RectCommand band;
	band.bounds = bandBounds;
	band.fill = theme.palette.ink;
	band.cornerRadius = 0.0;

	target.AddRect(band);

	TextStyle style = IndexNumberStyle(theme);
	style.align = TextAlign::Left;

	for (std::size_t index = 0; index < columnCount; ++index)
	{
		const Rect slot = Rect{
			ColumnOffset(theme.metrics, static_cast<int>(columnCount), static_cast<int>(index)),
			bandBounds.top + theme.metrics.card.padding,
			INDEX_COLUMN_WIDTH,
			theme.type.indexNumber};

		EmitSingleLine(target, FormatIndex(index), style, slot, measurer);
	}
}

void EmitStripColumns(
	DrawList& target,
	const std::vector<NumberedEntry>& columns,
	const Theme& theme,
	const Rect& area,
	const ITextMeasurer& measurer)
{
	const int columnCount = static_cast<int>(columns.size());
	const double columnWidth = ColumnWidth(theme.metrics, columnCount);

	for (std::size_t index = 0; index < columns.size(); ++index)
	{
		const Rect columnBounds = Rect{
			ColumnOffset(theme.metrics, columnCount, static_cast<int>(index)),
			area.top,
			columnWidth,
			area.height};

		const double titleHeight = EmitTextBlock(
			target, columns[index].title, CardTitleStyle(theme), columnBounds, measurer);

		const Rect bodyArea = DropTop(columnBounds, titleHeight + theme.metrics.card.titleToBodyGap);

		EmitTextBlock(target, columns[index].body, BodyStyle(theme), bodyArea, measurer);
	}
}

void EmitBulletList(
	DrawList& target,
	const std::vector<std::string>& bullets,
	const Theme& theme,
	const Rect& area,
	const ITextMeasurer& measurer)
{
	const TextStyle style = BodyStyle(theme);
	double cursor = area.top;

	for (const std::string& bullet : bullets)
	{
		RectCommand marker;
		marker.bounds = Rect{
			area.left,
			cursor + (style.lineHeight - BULLET_MARKER_SIZE) / 2.0,
			BULLET_MARKER_SIZE,
			BULLET_MARKER_SIZE};
		marker.fill = theme.palette.accent;
		marker.cornerRadius = 0.0;

		target.AddRect(marker);

		const Rect textArea = Rect{
			area.left + BULLET_MARKER_SIZE + BULLET_MARKER_GAP,
			cursor,
			area.width - BULLET_MARKER_SIZE - BULLET_MARKER_GAP,
			area.height};

		cursor += EmitTextBlock(target, bullet, style, textArea, measurer)
			+ theme.type.paragraphSpacing;
	}
}

DrawList BuildTitleSlide(
	const TitleSlideContent& content,
	const Theme& theme,
	const ITextMeasurer& measurer)
{
	DrawList result;

	EmitBackground(result, theme, theme.palette.background);
	EmitDecoration(result, theme, AssetRole::TitleDecoration, SlideBounds(theme));

	RectCommand band;
	band.bounds = Rect{
		0.0,
		theme.metrics.page.height - HERO_BAND_HEIGHT,
		theme.metrics.page.width,
		HERO_BAND_HEIGHT};
	band.fill = theme.palette.ink;
	band.cornerRadius = 0.0;

	result.AddRect(band);

	const Rect titleArea = Rect{
		theme.metrics.page.marginLeft,
		HERO_TITLE_TOP,
		ContentWidth(theme.metrics) / 2.0,
		theme.metrics.page.height - HERO_TITLE_TOP};

	EmitTextBlock(result, content.title, HeroTitleStyle(theme), titleArea, measurer);

	const Rect captionArea = Rect{
		theme.metrics.page.marginLeft,
		band.bounds.top + HERO_CAPTION_OFFSET,
		ContentWidth(theme.metrics),
		HERO_BAND_HEIGHT};

	EmitTextBlock(result, content.caption, CaptionStyle(theme), captionArea, measurer);

	return result;
}

DrawList BuildCardGridSlide(
	const CardGridSlideContent& content,
	const Theme& theme,
	int pageNumber,
	const ITextMeasurer& measurer)
{
	AssertIsNotEmpty(content.cards);

	DrawList result;

	EmitBackground(result, theme, theme.palette.background);

	const Rect sideBounds = Rect{
		theme.metrics.page.width - SIDE_DECORATION_WIDTH,
		0.0,
		SIDE_DECORATION_WIDTH,
		theme.metrics.page.height};

	EmitDecoration(result, theme, AssetRole::SideDecoration, sideBounds);
	EmitSlideTitle(result, theme, content.title, ContentWidth(theme.metrics), false, measurer);
	EmitCardGrid(result, content, theme, ContentArea(theme), measurer);
	EmitFooter(result, theme, pageNumber, false, measurer);

	return result;
}

DrawList BuildColumnStripSlide(
	const ColumnStripSlideContent& content,
	const Theme& theme,
	int pageNumber,
	const ITextMeasurer& measurer)
{
	AssertIsNotEmpty(content.columns);

	DrawList result;

	EmitBackground(result, theme, theme.palette.background);
	EmitSlideTitle(result, theme, content.title, ContentWidth(theme.metrics), false, measurer);

	const Rect bandBounds = Rect{
		0.0,
		theme.metrics.band.topOffset,
		theme.metrics.page.width,
		theme.metrics.band.height};

	const Rect panelBounds = Rect{
		0.0,
		RectBottom(bandBounds),
		theme.metrics.page.width,
		theme.metrics.page.height - RectBottom(bandBounds) - theme.metrics.page.marginBottom};

	RectCommand panel;
	panel.bounds = panelBounds;
	panel.fill = theme.palette.surface;
	panel.cornerRadius = 0.0;

	result.AddRect(panel);
	EmitIndexBand(result, content.columns.size(), theme, bandBounds, measurer);
	EmitStripSeparators(result, content.columns.size(), theme, InsetRect(panelBounds, theme.metrics.card.padding));

	const Rect columnsArea = Rect{
		theme.metrics.page.marginLeft,
		panelBounds.top + theme.metrics.card.padding,
		ContentWidth(theme.metrics),
		panelBounds.height - theme.metrics.card.padding * 2.0};

	EmitStripColumns(result, content.columns, theme, columnsArea, measurer);
	EmitFooter(result, theme, pageNumber, false, measurer);

	return result;
}

DrawList BuildBulletSlide(
	const BulletSlideContent& content,
	const Theme& theme,
	int pageNumber,
	const ITextMeasurer& measurer)
{
	DrawList result;

	EmitBackground(result, theme, theme.palette.background);

	const bool hasIllustration = content.illustration.has_value();
	const double halfWidth = theme.metrics.page.width / 2.0;

	if (hasIllustration)
	{
		ImageCommand illustration;
		illustration.bounds = Rect{0.0, 0.0, halfWidth, theme.metrics.page.height};
		illustration.source = content.illustration.value();

		result.AddImage(illustration);
	}

	const double textLeft = hasIllustration ? halfWidth + theme.metrics.page.marginLeft
											: theme.metrics.page.marginLeft;
	const double textWidth = theme.metrics.page.width - textLeft - theme.metrics.page.marginRight;

	TextStyle titleStyle = SlideTitleStyle(theme);

	const Rect titleArea = Rect{
		textLeft,
		theme.metrics.titleBlockTop,
		textWidth,
		theme.metrics.titleBlockHeight};

	const double titleHeight = EmitTextBlock(result, content.title, titleStyle, titleArea, measurer);

	const Rect bulletsArea = Rect{
		textLeft,
		titleArea.top + titleHeight + TITLE_TO_CONTENT_GAP,
		textWidth,
		theme.metrics.page.height - titleArea.top - titleHeight - theme.metrics.page.marginBottom};

	EmitBulletList(result, content.bullets, theme, bulletsArea, measurer);
	EmitFooter(result, theme, pageNumber, false, measurer);

	return result;
}

DrawList BuildClosingSlide(
	const ClosingSlideContent& content,
	const Theme& theme,
	const ITextMeasurer& measurer)
{
	DrawList result;

	EmitBackground(result, theme, theme.palette.ink);
	EmitDecoration(result, theme, AssetRole::FooterDecoration, SlideBounds(theme));
	EmitSlideTitle(result, theme, content.title, ContentWidth(theme.metrics) / 2.0, true, measurer);

	Rect cursorArea = Rect{
		theme.metrics.page.marginLeft,
		theme.metrics.contentTop,
		ContentWidth(theme.metrics) / 2.0,
		theme.metrics.page.height - theme.metrics.contentTop};

	for (const std::string& line : content.lines)
	{
		const double consumed = EmitTextBlock(result, line, BodyInverseStyle(theme), cursorArea, measurer);

		cursorArea = DropTop(cursorArea, consumed + theme.type.paragraphSpacing);
	}

	return result;
}
} // namespace

SlideLayoutEngine::SlideLayoutEngine(Theme theme, const ITextMeasurer& measurer)
	: m_theme(std::move(theme))
	, m_measurer(measurer)
{
}

DrawList SlideLayoutEngine::BuildSlide(const Slide& slide, int pageNumber) const
{
	return std::visit(
		Overloaded{
			[&](const TitleSlideContent& content) {
				return BuildTitleSlide(content, m_theme, m_measurer);
			},
			[&](const CardGridSlideContent& content) {
				return BuildCardGridSlide(content, m_theme, pageNumber, m_measurer);
			},
			[&](const ColumnStripSlideContent& content) {
				return BuildColumnStripSlide(content, m_theme, pageNumber, m_measurer);
			},
			[&](const BulletSlideContent& content) {
				return BuildBulletSlide(content, m_theme, pageNumber, m_measurer);
			},
			[&](const ClosingSlideContent& content) {
				return BuildClosingSlide(content, m_theme, m_measurer);
			}},
		slide.content);
}

std::vector<DrawList> SlideLayoutEngine::BuildDeck(const Deck& deck) const
{
	std::vector<DrawList> pages;
	pages.reserve(deck.slides.size());

	int pageNumber = 1;

	for (const Slide& slide : deck.slides)
	{
		pages.push_back(BuildSlide(slide, pageNumber));
		++pageNumber;
	}

	return pages;
}
