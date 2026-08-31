#include "SlideLayoutEngine.hpp"
#include "SlideChrome.hpp"
#include "common/output/pdf/graphics/ChartPainter.hpp"
#include "common/output/pdf/model/Slide.hpp"
#include "common/output/pdf/theme/TextStyles.hpp"
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
constexpr double KPI_VALUE_TO_LABEL_GAP = 10.0;
constexpr double KPI_ROW_GAP = 26.0;
constexpr double COMMENTARY_TOP_GAP = 30.0;
constexpr double SOURCE_NOTE_HEIGHT = 16.0;
constexpr double SOURCE_NOTE_BOTTOM = 34.0;
constexpr double TAKEAWAY_HEIGHT = 54.0;
constexpr double TAKEAWAY_GAP = 18.0;
constexpr double CHART_NOTES_WIDTH = 210.0;
constexpr double CHART_NOTES_GAP = 26.0;
constexpr double TABLE_ROW_PADDING = 9.0;
constexpr double TABLE_HEADER_EXTRA = 4.0;
constexpr int KPI_COLUMNS = 4;
constexpr double KPI_VALUE_SCALE = 0.62;
constexpr double KPI_LABEL_TO_VALUE_GAP = 12.0;
constexpr double KPI_MIN_VALUE_SCALE = 0.55;

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

TextStyle LabelStyle(const Theme& theme)
{
	TextStyle style = CaptionStyle(theme);
	style.color = theme.palette.text;
	style.align = TextAlign::Left;

	return style;
}

double KpiRowHeight(const Theme& theme)
{
	return CaptionStyle(theme).lineHeight
		+ KPI_LABEL_TO_VALUE_GAP
		+ theme.type.heroNumber * KPI_VALUE_SCALE * theme.type.lineHeightFactor
		+ KPI_VALUE_TO_LABEL_GAP
		+ BodyStyle(theme).lineHeight;
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

	TextStyle style = LabelStyle(theme);

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

void EmitKpiFigures(
	DrawList& target,
	const std::vector<KpiEntry>& figures,
	const Theme& theme,
	const Rect& area,
	const ITextMeasurer& measurer)
{
	const int columnCount = std::min(KPI_COLUMNS, static_cast<int>(figures.size()));
	if (columnCount == 0)
	{
		return;
	}

	const double columnWidth = ColumnWidth(theme.metrics, columnCount);
	const double rowHeight = KpiRowHeight(theme);

	TextStyle labelStyle = LabelStyle(theme);

	TextStyle valueStyle = HeroTitleStyle(theme);
	valueStyle.fontSize = theme.type.heroNumber * KPI_VALUE_SCALE;
	valueStyle.lineHeight = valueStyle.fontSize * theme.type.lineHeightFactor;
	valueStyle.align = TextAlign::Left;

	TextStyle noteStyle = LabelStyle(theme);

	const double valueWidth = columnWidth - theme.metrics.card.columnGap;
	for (const auto& figure : figures)
	{
		const double measured = measurer.MeasureWidth(
			figure.value, valueStyle.font, valueStyle.fontSize);

		if (measured <= valueWidth || measured <= 0.0)
		{
			continue;
		}

		const double scale = std::max(valueWidth / measured, KPI_MIN_VALUE_SCALE);
		valueStyle.fontSize *= scale;
		valueStyle.lineHeight = valueStyle.fontSize * theme.type.lineHeightFactor;
	}

	for (std::size_t index = 0; index < figures.size(); ++index)
	{
		const int column = static_cast<int>(index) % columnCount;
		const int row = static_cast<int>(index) / columnCount;

		const double top = area.top + static_cast<double>(row) * (rowHeight + KPI_ROW_GAP);

		const Rect labelArea = Rect{
			ColumnOffset(theme.metrics, columnCount, column),
			top,
			columnWidth,
			labelStyle.lineHeight};

		EmitSingleLine(target, figures[index].label, labelStyle, labelArea, measurer);

		const Rect valueArea = Rect{
			labelArea.left,
			top + labelStyle.lineHeight + KPI_LABEL_TO_VALUE_GAP,
			valueWidth,
			valueStyle.lineHeight};

		EmitSingleLine(target, figures[index].value, valueStyle, valueArea, measurer);

		LineCommand underline;
		underline.from = Point{labelArea.left, RectBottom(valueArea) + KPI_VALUE_TO_LABEL_GAP / 2.0};
		underline.to = Point{labelArea.left + columnWidth - theme.metrics.card.columnGap,
			RectBottom(valueArea) + KPI_VALUE_TO_LABEL_GAP / 2.0};
		underline.stroke = theme.palette.accent;
		underline.thickness = SEPARATOR_THICKNESS * 2.0;

		target.AddLine(underline);

		if (figures[index].note.empty())
		{
			continue;
		}

		const Rect noteArea = Rect{
			labelArea.left,
			RectBottom(valueArea) + KPI_VALUE_TO_LABEL_GAP,
			columnWidth - theme.metrics.card.columnGap,
			noteStyle.lineHeight};

		EmitSingleLine(target, figures[index].note, noteStyle, noteArea, measurer);
	}
}

DrawList BuildKpiSlide(
	const KpiSlideContent& content,
	const Theme& theme,
	int pageNumber,
	const ITextMeasurer& measurer)
{
	DrawList result;

	EmitBackground(result, theme, theme.palette.background);
	EmitSlideTitle(result, theme, content.title, ContentWidth(theme.metrics), false, measurer);

	const Rect area = ContentArea(theme);
	EmitKpiFigures(result, content.figures, theme, area, measurer);

	if (!content.commentary.empty())
	{
		const int columnCount = std::min(KPI_COLUMNS, static_cast<int>(content.figures.size()));
		const int rowCount = columnCount == 0
			? 0
			: (static_cast<int>(content.figures.size()) + columnCount - 1) / columnCount;

		const Rect commentaryArea = DropTop(
			area,
			static_cast<double>(rowCount) * (KpiRowHeight(theme) + KPI_ROW_GAP)
				+ COMMENTARY_TOP_GAP);

		EmitTextBlock(result, content.commentary, BodyStyle(theme), commentaryArea, measurer);
	}

	EmitSourceNote(result, content.sourceNote, theme, measurer);
	EmitFooter(result, theme, pageNumber, false, measurer);

	return result;
}

DrawList BuildChartSlide(
	const ChartSlideContent& content,
	const Theme& theme,
	int pageNumber,
	const ITextMeasurer& measurer)
{
	DrawList result;

	EmitBackground(result, theme, theme.palette.background);
	EmitSlideTitle(result, theme, content.title, ContentWidth(theme.metrics), false, measurer);

	const Rect area = ContentArea(theme);
	const double bottom = theme.metrics.page.height - SOURCE_NOTE_BOTTOM - SOURCE_NOTE_HEIGHT;
	const bool hasTakeaway = !content.takeaway.empty();
	const bool hasNotes = !content.notes.empty();

	const double chartBottom = hasTakeaway ? bottom - TAKEAWAY_HEIGHT - TAKEAWAY_GAP : bottom;
	const double chartWidth = hasNotes ? area.width - CHART_NOTES_WIDTH - CHART_NOTES_GAP
									   : area.width;

	const Rect chartArea = Rect{area.left, area.top, chartWidth, chartBottom - area.top};

	const ChartPainter painter(theme, measurer);
	painter.Paint(result, content, chartArea);

	if (hasNotes)
	{
		const Rect notesArea = Rect{
			RectRight(area) - CHART_NOTES_WIDTH,
			area.top,
			CHART_NOTES_WIDTH,
			chartArea.height};

		EmitBulletList(result, content.notes, theme, notesArea, measurer);
	}

	if (hasTakeaway)
	{
		const Rect takeawayArea = Rect{
			area.left,
			chartBottom + TAKEAWAY_GAP,
			area.width,
			TAKEAWAY_HEIGHT};

		EmitTakeaway(result, content.takeaway, theme, takeawayArea, measurer);
	}

	EmitSourceNote(result, content.sourceNote, theme, measurer);
	EmitFooter(result, theme, pageNumber, false, measurer);

	return result;
}

void EmitTableRow(
	DrawList& target,
	const std::vector<std::string>& cells,
	const TextStyle& style,
	const Theme& theme,
	const Rect& rowBounds,
	const ITextMeasurer& measurer)
{
	const int columnCount = static_cast<int>(cells.size());
	const double columnWidth = rowBounds.width / static_cast<double>(columnCount);

	for (std::size_t index = 0; index < cells.size(); ++index)
	{
		TextStyle cellStyle = style;
		cellStyle.align = index == 0 ? TextAlign::Left : TextAlign::Right;

		const Rect cellArea = Rect{
			rowBounds.left + columnWidth * static_cast<double>(index)
				+ theme.metrics.card.padding / 2.0,
			rowBounds.top + TABLE_ROW_PADDING,
			columnWidth - theme.metrics.card.padding,
			style.lineHeight};

		EmitSingleLine(target, cells[index], cellStyle, cellArea, measurer);
	}
}

DrawList BuildTableSlide(
	const TableSlideContent& content,
	const Theme& theme,
	int pageNumber,
	const ITextMeasurer& measurer)
{
	DrawList result;

	EmitBackground(result, theme, theme.palette.background);
	EmitSlideTitle(result, theme, content.title, ContentWidth(theme.metrics), false, measurer);

	const Rect area = ContentArea(theme);
	const TextStyle bodyStyle = BodyStyle(theme);
	const double rowHeight = bodyStyle.lineHeight + TABLE_ROW_PADDING * 2.0;

	TextStyle headerStyle = CardTitleStyle(theme);
	headerStyle.fontSize = theme.type.body;
	headerStyle.lineHeight = bodyStyle.lineHeight;

	const Rect headerBounds = Rect{
		area.left,
		area.top,
		area.width,
		rowHeight + TABLE_HEADER_EXTRA};

	RectCommand headerBackground;
	headerBackground.bounds = headerBounds;
	headerBackground.fill = theme.palette.surface;
	headerBackground.cornerRadius = 0.0;

	result.AddRect(headerBackground);
	EmitTableRow(result, content.headers, headerStyle, theme, headerBounds, measurer);

	double cursor = RectBottom(headerBounds);

	for (std::size_t index = 0; index < content.rows.size(); ++index)
	{
		const Rect rowBounds = Rect{area.left, cursor, area.width, rowHeight};

		if (index % 2 == 1)
		{
			RectCommand stripe;
			stripe.bounds = rowBounds;
			stripe.fill = theme.palette.surface;
			stripe.cornerRadius = 0.0;

			result.AddRect(stripe);
		}

		EmitTableRow(result, content.rows[index], bodyStyle, theme, rowBounds, measurer);

		LineCommand separator;
		separator.from = Point{area.left, RectBottom(rowBounds)};
		separator.to = Point{RectRight(area), RectBottom(rowBounds)};
		separator.stroke = theme.palette.accentSoft;
		separator.thickness = SEPARATOR_THICKNESS;

		result.AddLine(separator);

		cursor += rowHeight;
	}

	if (!content.takeaway.empty())
	{
		const Rect takeawayArea = Rect{
			area.left,
			cursor + TAKEAWAY_GAP,
			area.width,
			TAKEAWAY_HEIGHT};

		EmitTakeaway(result, content.takeaway, theme, takeawayArea, measurer);
	}

	EmitSourceNote(result, content.sourceNote, theme, measurer);
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
			[&](const KpiSlideContent& content) {
				return BuildKpiSlide(content, m_theme, pageNumber, m_measurer);
			},
			[&](const ChartSlideContent& content) {
				return BuildChartSlide(content, m_theme, pageNumber, m_measurer);
			},
			[&](const TableSlideContent& content) {
				return BuildTableSlide(content, m_theme, pageNumber, m_measurer);
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