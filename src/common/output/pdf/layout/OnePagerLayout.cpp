#include "OnePagerLayout.hpp"

#include "SlideChrome.hpp"
#include "TextBlock.hpp"
#include "common/output/pdf/graphics/ChartPainter.hpp"
#include "common/output/pdf/theme/TextStyles.hpp"

#include <algorithm>
#include <cctype>

namespace
{
constexpr double MeasurementScratchHeight = 100000.0;
constexpr double RowGap = 16.0;

constexpr double EyebrowToHeadlineGap = 10.0;
constexpr double HeadlineToSubtitleGap = 8.0;
constexpr double SubtitleToRuleGap = 14.0;
constexpr double HeroRuleThickness = 1.0;
constexpr double RuleToBodyGap = 18.0;
constexpr double HeadlineWidthFraction = 0.86;
constexpr std::size_t ShortHeadlineMaxWords = 6;
constexpr double ShortHeadlineFontScale = 0.7;

constexpr double ValuationColumnFraction = 0.30;
constexpr double KpiToMixGap = 14.0;
constexpr double RevenueMixBarHeight = 18.0;
constexpr double RevenueMixLabelGap = 4.0;
constexpr double KpiValueScale = 0.40;
constexpr double KpiMinValueScale = 0.5;
constexpr double KpiLabelToValueGap = 6.0;
constexpr double KpiValueToNoteGap = 4.0;
constexpr double KpiUnderlineThickness = 1.5;

constexpr double BridgeConnectorHeight = 8.0;
constexpr double BridgeBoxPadding = 6.0;
constexpr double BridgeLabelToValueGap = 4.0;
constexpr double BridgeConnectorThickness = 1.0;
constexpr double BridgeNoteGap = 6.0;
constexpr double BridgeValueFontSize = 16.0;

constexpr double ChartColumnFraction = 0.45;
constexpr double FactsColumnFraction = 0.20;
constexpr double ChartTitleGap = 8.0;
constexpr double ChartMinHeight = 210.0;
constexpr double FactGap = 10.0;
constexpr double FactTitleToBodyGap = 4.0;
constexpr double ThesisGap = 12.0;
constexpr double ThesisTitleToBodyGap = 6.0;

constexpr double RisksToNextStepGap = 8.0;
constexpr double RiskTitleToBodyGap = 12.0;
constexpr double NextStepPadding = 10.0;

double LocalColumnWidth(const double containerWidth, const int columnCount, const double gap)
{
	return (containerWidth - gap * static_cast<double>(columnCount - 1)) / static_cast<double>(columnCount);
}

double LocalColumnOffset(const double containerLeft, const double columnWidth, const double gap, const int columnIndex)
{
	return containerLeft + (columnWidth + gap) * static_cast<double>(columnIndex);
}

std::size_t CountWords(const std::string& text)
{
	std::size_t count = 0;
	bool insideWord = false;

	for (const char character : text)
	{
		const bool isSpace = std::isspace(static_cast<unsigned char>(character)) != 0;
		if (!isSpace && !insideWord)
		{
			++count;
		}

		insideWord = !isSpace;
	}

	return count;
}

std::string JoinValueCreationSteps(const std::vector<std::string>& steps)
{
	std::string joined;

	for (std::size_t index = 0; index < steps.size(); ++index)
	{
		if (index > 0)
		{
			joined += "  ->  ";
		}

		joined += steps[index];
	}

	return joined;
}
} // namespace

OnePagerLayout::OnePagerLayout(const Theme& theme, const ITextMeasurer& measurer)
	: m_theme(theme)
	, m_measurer(measurer)
{
}

Rect OnePagerLayout::PaintHero(
	DrawList& target,
	const OnePagerSlideContent& content,
	const Rect& area) const
{
	TextStyle eyebrowStyle = CaptionStyle(m_theme);
	eyebrowStyle.color = m_theme.palette.text;
	TextStyle vintageStyle = eyebrowStyle;
	vintageStyle.align = TextAlign::Right;

	const Rect eyebrowRow = TakeTop(area, eyebrowStyle.lineHeight);
	EmitSingleLine(target, content.eyebrow, eyebrowStyle, eyebrowRow, m_measurer);
	EmitSingleLine(target, content.vintageLabel, vintageStyle, eyebrowRow, m_measurer);

	Rect cursor = DropTop(area, eyebrowStyle.lineHeight + EyebrowToHeadlineGap);

	TextStyle headlineStyle = SlideTitleStyle(m_theme);
	const std::size_t headlineWordCount = CountWords(content.headline);
	if (headlineWordCount >= 1 && headlineWordCount <= ShortHeadlineMaxWords)
	{
		headlineStyle.fontSize *= ShortHeadlineFontScale;
		headlineStyle.lineHeight = headlineStyle.fontSize * m_theme.type.lineHeightFactor;
	}

	const double headlineWidth = area.width * HeadlineWidthFraction;
	const double headlineHeight = MeasureTextHeight(content.headline, headlineStyle, headlineWidth, m_measurer);

	EmitTextBlock(
		target, content.headline, headlineStyle,
		Rect{cursor.left, cursor.top, headlineWidth, headlineHeight}, m_measurer);

	cursor = DropTop(cursor, headlineHeight + HeadlineToSubtitleGap);

	const TextStyle subtitleStyle = BodyStyle(m_theme);
	EmitSingleLine(
		target, content.subtitle, subtitleStyle, TakeTop(cursor, subtitleStyle.lineHeight), m_measurer);
	cursor = DropTop(cursor, subtitleStyle.lineHeight + SubtitleToRuleGap);

	LineCommand rule;
	rule.from = Point{area.left, cursor.top};
	rule.to = Point{RectRight(area), cursor.top};
	rule.stroke = m_theme.palette.text;
	rule.thickness = HeroRuleThickness;
	target.AddLine(rule);

	return DropTop(cursor, HeroRuleThickness + RuleToBodyGap);
}

double OnePagerLayout::MeasureRevenueMixHeight(const std::vector<RevenueMixSegment>& segments, const double areaWidth) const
{
	if (segments.empty())
	{
		return 0.0;
	}

	const TextStyle labelStyle = CaptionStyle(m_theme);
	const double labelWidth = areaWidth / static_cast<double>(segments.size());

	double tallestLabel = labelStyle.lineHeight;

	for (const auto& segment : segments)
	{
		const std::string text = segment.name + " \xC2\xB7 " + segment.share + " \xC2\xB7 " + segment.amount;
		tallestLabel = std::max(tallestLabel, MeasureTextHeight(text, labelStyle, labelWidth, m_measurer));
	}

	return tallestLabel + RevenueMixLabelGap + RevenueMixBarHeight;
}

void OnePagerLayout::PaintRevenueMix(
	DrawList& target,
	const std::vector<RevenueMixSegment>& segments,
	const Rect& area) const
{
	if (segments.empty())
	{
		return;
	}

	const std::vector<Color> palette{m_theme.palette.accent, m_theme.palette.ink, m_theme.palette.violet};

	TextStyle labelStyle = CaptionStyle(m_theme);
	labelStyle.color = m_theme.palette.text;

	const double labelWidth = area.width / static_cast<double>(segments.size());
	const double labelHeight = std::max(0.0, area.height - RevenueMixLabelGap - RevenueMixBarHeight);

	const Rect barArea = Rect{
		area.left, area.top + labelHeight + RevenueMixLabelGap, area.width, RevenueMixBarHeight};

	double barCursor = barArea.left;
	double labelCursor = area.left;

	for (std::size_t index = 0; index < segments.size(); ++index)
	{
		const double segmentWidth = barArea.width * segments[index].shareFraction;

		RectCommand segmentRect;
		segmentRect.bounds = Rect{barCursor, barArea.top, segmentWidth, barArea.height};
		segmentRect.fill = palette[index % palette.size()];
		segmentRect.cornerRadius = 0.0;
		target.AddRect(segmentRect);
		barCursor += segmentWidth;

		const std::string text = segments[index].name + " \xC2\xB7 " + segments[index].share
			+ " \xC2\xB7 " + segments[index].amount;
		EmitTextBlock(
			target, text, labelStyle, Rect{labelCursor, area.top, labelWidth, labelHeight}, m_measurer);
		labelCursor += labelWidth;
	}
}

TextStyle OnePagerLayout::MeasureKpiValueStyle(const std::vector<KpiEntry>& kpis, const double tileWidth) const
{
	TextStyle valueStyle = HeroTitleStyle(m_theme);
	valueStyle.fontSize = m_theme.type.heroNumber * KpiValueScale;
	valueStyle.lineHeight = valueStyle.fontSize * m_theme.type.lineHeightFactor;

	for (const auto& figure : kpis)
	{
		const double measured = m_measurer.MeasureWidth(figure.value, valueStyle.font, valueStyle.fontSize);
		if (measured <= tileWidth || measured <= 0.0)
		{
			continue;
		}

		const double scale = std::max(tileWidth / measured, KpiMinValueScale);
		valueStyle.fontSize *= scale;
		valueStyle.lineHeight = valueStyle.fontSize * m_theme.type.lineHeightFactor;
	}

	return valueStyle;
}

double OnePagerLayout::MeasureKpiTileHeight(const std::vector<KpiEntry>& kpis, const TextStyle& valueStyle) const
{
	if (kpis.empty())
	{
		return 0.0;
	}

	const TextStyle labelStyle = CaptionStyle(m_theme);
	const bool hasNote = std::any_of(
		kpis.begin(), kpis.end(), [](const KpiEntry& kpi) { return !kpi.note.empty(); });

	double height = labelStyle.lineHeight + KpiLabelToValueGap + valueStyle.lineHeight;

	if (hasNote)
	{
		height += KpiValueToNoteGap + labelStyle.lineHeight;
	}

	return height;
}

double OnePagerLayout::MeasureKpiAndValuationRowHeight(const OnePagerSlideContent& content, const double rowWidth) const
{
	const double gap = m_theme.metrics.card.columnGap;
	const bool hasBridge = !content.valuationBridge.empty();
	const double valuationWidth = hasBridge ? rowWidth * ValuationColumnFraction : 0.0;
	const double kpiWidth = hasBridge ? rowWidth - valuationWidth - gap : rowWidth;

	double kpiColumnHeight = 0.0;

	if (!content.kpis.empty())
	{
		const int count = static_cast<int>(content.kpis.size());
		const double tileWidth = LocalColumnWidth(kpiWidth, count, gap);
		const TextStyle valueStyle = MeasureKpiValueStyle(content.kpis, tileWidth);

		kpiColumnHeight = MeasureKpiTileHeight(content.kpis, valueStyle);

		if (!content.revenueMix.empty())
		{
			kpiColumnHeight += KpiToMixGap + MeasureRevenueMixHeight(content.revenueMix, kpiWidth);
		}
	}

	const double valuationHeight = hasBridge
		? MeasureValuationBridgeHeight(content.valuationBridge, content.valuationMultipleNote)
		: 0.0;

	return std::max(kpiColumnHeight, valuationHeight);
}

void OnePagerLayout::PaintKpiAndValuationRow(
	DrawList& target,
	const OnePagerSlideContent& content,
	const Rect& area) const
{
	const double gap = m_theme.metrics.card.columnGap;
	const bool hasBridge = !content.valuationBridge.empty();
	const double valuationWidth = hasBridge ? area.width * ValuationColumnFraction : 0.0;
	const double kpiWidth = hasBridge ? area.width - valuationWidth - gap : area.width;

	const Rect kpiColumn = Rect{area.left, area.top, kpiWidth, area.height};

	if (!content.kpis.empty())
	{
		const int count = static_cast<int>(content.kpis.size());
		const double tileWidth = LocalColumnWidth(kpiColumn.width, count, gap);

		TextStyle labelStyle = CaptionStyle(m_theme);
		labelStyle.color = m_theme.palette.text;

		const TextStyle valueStyle = MeasureKpiValueStyle(content.kpis, tileWidth);
		const TextStyle noteStyle = labelStyle;
		const double tileHeight = MeasureKpiTileHeight(content.kpis, valueStyle);

		for (int index = 0; index < count; ++index)
		{
			const double left = LocalColumnOffset(kpiColumn.left, tileWidth, gap, index);

			const Rect labelArea = Rect{left, kpiColumn.top, tileWidth, labelStyle.lineHeight};
			EmitSingleLine(target, content.kpis[index].label, labelStyle, labelArea, m_measurer);

			TextStyle thisValueStyle = valueStyle;
			thisValueStyle.color = content.kpis[index].emphasizeNegative
				? m_theme.palette.danger
				: m_theme.palette.text;

			const Rect valueArea = Rect{
				left, RectBottom(labelArea) + KpiLabelToValueGap, tileWidth, valueStyle.lineHeight};
			EmitSingleLine(target, content.kpis[index].value, thisValueStyle, valueArea, m_measurer);

			LineCommand underline;
			underline.from = Point{left, RectBottom(valueArea) + KpiValueToNoteGap / 2.0};
			underline.to = Point{left + tileWidth, RectBottom(valueArea) + KpiValueToNoteGap / 2.0};
			underline.stroke = m_theme.palette.accent;
			underline.thickness = KpiUnderlineThickness;
			target.AddLine(underline);

			if (!content.kpis[index].note.empty())
			{
				const Rect noteArea = Rect{
					left, RectBottom(valueArea) + KpiValueToNoteGap, tileWidth, noteStyle.lineHeight};
				EmitSingleLine(target, content.kpis[index].note, noteStyle, noteArea, m_measurer);
			}
		}

		if (!content.revenueMix.empty())
		{
			PaintRevenueMix(target, content.revenueMix, DropTop(kpiColumn, tileHeight + KpiToMixGap));
		}
	}

	if (hasBridge)
	{
		const Rect valuationColumn = Rect{area.left + kpiWidth + gap, area.top, valuationWidth, area.height};
		PaintValuationBridge(target, content.valuationBridge, content.valuationMultipleNote, valuationColumn);
	}
}

double OnePagerLayout::MeasureValuationBridgeHeight(
	const std::vector<ValuationBridgeStep>& steps,
	const std::string& note) const
{
	if (steps.empty())
	{
		return 0.0;
	}

	const TextStyle captionStyle = CaptionStyle(m_theme);
	const double valueLineHeight = BridgeValueFontSize * m_theme.type.lineHeightFactor;
	const double boxHeight = BridgeBoxPadding * 2.0 + captionStyle.lineHeight + BridgeLabelToValueGap + valueLineHeight;
	const double stepCount = static_cast<double>(steps.size());

	double total = stepCount * boxHeight + (stepCount - 1.0) * BridgeConnectorHeight;

	if (!note.empty())
	{
		total += captionStyle.lineHeight + BridgeNoteGap;
	}

	return total;
}

void OnePagerLayout::PaintValuationBridge(
	DrawList& target,
	const std::vector<ValuationBridgeStep>& steps,
	const std::string& note,
	const Rect& area) const
{
	if (steps.empty())
	{
		return;
	}

	const int stepCount = static_cast<int>(steps.size());
	const TextStyle captionStyle = CaptionStyle(m_theme);

	TextStyle valueStyle = CardTitleStyle(m_theme);
	valueStyle.fontSize = BridgeValueFontSize;
	valueStyle.lineHeight = BridgeValueFontSize * m_theme.type.lineHeightFactor;

	const double boxHeight = BridgeBoxPadding * 2.0 + captionStyle.lineHeight + BridgeLabelToValueGap + valueStyle.lineHeight;

	double top = area.top;

	for (int index = 0; index < stepCount; ++index)
	{
		const Rect box = Rect{area.left, top, area.width, boxHeight};
		const bool isTotal = steps[index].isTotal;

		RectCommand panel;
		panel.bounds = box;
		panel.fill = isTotal ? m_theme.palette.ink : m_theme.palette.surface;
		panel.cornerRadius = m_theme.metrics.card.cornerRadius;
		target.AddRect(panel);

		TextStyle labelStyle = captionStyle;
		labelStyle.color = isTotal ? m_theme.palette.textInverse : m_theme.palette.text;
		TextStyle thisValueStyle = valueStyle;
		thisValueStyle.color = isTotal ? m_theme.palette.textInverse : m_theme.palette.text;

		const Rect inner = InsetRect(box, BridgeBoxPadding);
		const std::string labelLine = steps[index].annotation.empty()
			? steps[index].label
			: steps[index].label + "   \xC2\xB7   " + steps[index].annotation;

		EmitSingleLine(target, labelLine, labelStyle, TakeTop(inner, labelStyle.lineHeight), m_measurer);
		EmitSingleLine(
			target, steps[index].value, thisValueStyle,
			DropTop(inner, labelStyle.lineHeight + BridgeLabelToValueGap), m_measurer);

		top += boxHeight;

		if (index + 1 < stepCount)
		{
			LineCommand connector;
			connector.from = Point{box.left + box.width / 2.0, top};
			connector.to = Point{box.left + box.width / 2.0, top + BridgeConnectorHeight};
			connector.stroke = m_theme.palette.text;
			connector.thickness = BridgeConnectorThickness;
			target.AddLine(connector);

			top += BridgeConnectorHeight;
		}
	}

	if (!note.empty())
	{
		EmitSingleLine(
			target, note, captionStyle,
			Rect{area.left, top + BridgeNoteGap, area.width, captionStyle.lineHeight}, m_measurer);
	}
}

double OnePagerLayout::MeasureChartBusinessThesisRowHeight(
	const OnePagerSlideContent& content,
	const double rowWidth) const
{
	const double gap = m_theme.metrics.card.columnGap;
	const double usable = rowWidth - gap * 2.0;
	const double factsWidth = usable * FactsColumnFraction;
	const double thesisWidth = usable - usable * ChartColumnFraction - factsWidth;

	double tallest = content.trajectory.has_value() ? ChartMinHeight : 0.0;

	if (!content.businessFacts.empty())
	{
		const TextStyle labelStyle = CaptionStyle(m_theme);
		const TextStyle valueStyle = CardTitleStyle(m_theme);

		double height = 0.0;
		for (const auto& fact : content.businessFacts)
		{
			height += labelStyle.lineHeight + FactTitleToBodyGap;
			height += MeasureTextHeight(fact.body, valueStyle, factsWidth, m_measurer);
			height += FactGap;
		}

		tallest = std::max(tallest, height);
	}

	if (!content.investmentCase.empty())
	{
		const TextStyle titleStyle = CardTitleStyle(m_theme);
		const TextStyle bodyStyle = BodyStyle(m_theme);

		double height = 0.0;
		for (const auto& thesis : content.investmentCase)
		{
			height += MeasureTextHeight(thesis.title, titleStyle, thesisWidth, m_measurer);
			height += ThesisTitleToBodyGap;
			height += MeasureTextHeight(thesis.body, bodyStyle, thesisWidth, m_measurer);
			height += ThesisGap;
		}

		if (!content.valueCreationSteps.empty())
		{
			const TextStyle stepsStyle = CaptionStyle(m_theme);
			const std::string joined = JoinValueCreationSteps(content.valueCreationSteps);
			height += MeasureTextHeight(joined, stepsStyle, thesisWidth, m_measurer);
		}

		tallest = std::max(tallest, height);
	}

	return tallest;
}

void OnePagerLayout::PaintChartBusinessThesisRow(
	DrawList& target,
	const OnePagerSlideContent& content,
	const Rect& area) const
{
	const double gap = m_theme.metrics.card.columnGap;
	const double usable = area.width - gap * 2.0;
	const double chartWidth = usable * ChartColumnFraction;
	const double factsWidth = usable * FactsColumnFraction;
	const double thesisWidth = usable - chartWidth - factsWidth;

	const Rect chartColumn = Rect{area.left, area.top, chartWidth, area.height};
	const Rect factsColumn = Rect{RectRight(chartColumn) + gap, area.top, factsWidth, area.height};
	const Rect thesisColumn = Rect{RectRight(factsColumn) + gap, area.top, thesisWidth, area.height};

	if (content.trajectory.has_value())
	{
		const TextStyle titleStyle = CardTitleStyle(m_theme);
		const Rect titleArea = TakeTop(chartColumn, titleStyle.lineHeight);
		EmitSingleLine(target, content.trajectory->title, titleStyle, titleArea, m_measurer);

		const Rect plotArea = DropTop(chartColumn, titleStyle.lineHeight + ChartTitleGap);
		const ChartPainter painter(m_theme, m_measurer);
		painter.Paint(target, *content.trajectory, plotArea);
	}

	if (!content.businessFacts.empty())
	{
		TextStyle labelStyle = CaptionStyle(m_theme);
		labelStyle.color = m_theme.palette.text;
		const TextStyle valueStyle = CardTitleStyle(m_theme);

		double cursor = factsColumn.top;

		for (const auto& fact : content.businessFacts)
		{
			EmitSingleLine(
				target, fact.title, labelStyle,
				Rect{factsColumn.left, cursor, factsColumn.width, labelStyle.lineHeight}, m_measurer);
			cursor += labelStyle.lineHeight + FactTitleToBodyGap;

			const double valueHeight = MeasureTextHeight(fact.body, valueStyle, factsColumn.width, m_measurer);
			EmitTextBlock(
				target, fact.body, valueStyle,
				Rect{factsColumn.left, cursor, factsColumn.width, valueHeight}, m_measurer);
			cursor += valueHeight + FactGap;
		}
	}

	if (!content.investmentCase.empty())
	{
		const TextStyle titleStyle = CardTitleStyle(m_theme);
		const TextStyle bodyStyle = BodyStyle(m_theme);
		double cursor = thesisColumn.top;

		for (const auto& thesis : content.investmentCase)
		{
			const double titleHeight = MeasureTextHeight(thesis.title, titleStyle, thesisColumn.width, m_measurer);
			EmitTextBlock(
				target, thesis.title, titleStyle,
				Rect{thesisColumn.left, cursor, thesisColumn.width, titleHeight}, m_measurer);
			cursor += titleHeight + ThesisTitleToBodyGap;

			const double bodyHeight = MeasureTextHeight(thesis.body, bodyStyle, thesisColumn.width, m_measurer);
			EmitTextBlock(
				target, thesis.body, bodyStyle,
				Rect{thesisColumn.left, cursor, thesisColumn.width, bodyHeight}, m_measurer);
			cursor += bodyHeight + ThesisGap;
		}

		if (!content.valueCreationSteps.empty())
		{
			const std::string joined = JoinValueCreationSteps(content.valueCreationSteps);

			const TextStyle stepsStyle = CaptionStyle(m_theme);
			const double stepsHeight = MeasureTextHeight(joined, stepsStyle, thesisColumn.width, m_measurer);
			EmitTextBlock(
				target, joined, stepsStyle,
				Rect{thesisColumn.left, cursor, thesisColumn.width, stepsHeight}, m_measurer);
		}
	}
}

double OnePagerLayout::MeasureRisksAndNextStepRowHeight(
	const OnePagerSlideContent& content,
	const double rowWidth) const
{
	double risksHeight = 0.0;

	if (!content.risks.empty())
	{
		const double gap = m_theme.metrics.card.columnGap;
		const int count = static_cast<int>(content.risks.size());
		const double columnWidth = LocalColumnWidth(rowWidth, count, gap);

		const TextStyle titleStyle = CardTitleStyle(m_theme);
		const TextStyle bodyStyle = CaptionStyle(m_theme);

		for (const auto& risk : content.risks)
		{
			const double titleHeight = MeasureTextHeight(risk.title, titleStyle, columnWidth, m_measurer);
			const double bodyHeight = MeasureTextHeight(risk.body, bodyStyle, columnWidth, m_measurer);
			risksHeight = std::max(risksHeight, titleHeight + RiskTitleToBodyGap + bodyHeight);
		}
	}

	if (content.nextStep.empty())
	{
		return risksHeight;
	}

	const TextStyle nextStepStyle = BodyInverseStyle(m_theme);
	const double nextStepHeight = nextStepStyle.lineHeight + NextStepPadding * 2.0;

	return risksHeight > 0.0 ? risksHeight + RisksToNextStepGap + nextStepHeight : nextStepHeight;
}

void OnePagerLayout::PaintRisksAndNextStepRow(
	DrawList& target,
	const OnePagerSlideContent& content,
	const Rect& area) const
{
	const TextStyle nextStepStyle = BodyInverseStyle(m_theme);
	const bool hasNextStep = !content.nextStep.empty();
	const double nextStepHeight = hasNextStep
		? nextStepStyle.lineHeight + NextStepPadding * 2.0
		: 0.0;
	const double risksHeight = hasNextStep
		? std::max(0.0, area.height - nextStepHeight - RisksToNextStepGap)
		: area.height;

	const Rect risksArea = TakeTop(area, risksHeight);
	const Rect nextStepArea = Rect{area.left, RectBottom(area) - nextStepHeight, area.width, nextStepHeight};

	if (!content.risks.empty())
	{
		const double gap = m_theme.metrics.card.columnGap;
		const int count = static_cast<int>(content.risks.size());
		const double columnWidth = LocalColumnWidth(risksArea.width, count, gap);

		TextStyle titleStyle = CardTitleStyle(m_theme);
		titleStyle.color = m_theme.palette.danger;

		TextStyle bodyStyle = CaptionStyle(m_theme);
		bodyStyle.color = m_theme.palette.text;

		for (int index = 0; index < count; ++index)
		{
			const double left = LocalColumnOffset(risksArea.left, columnWidth, gap, index);
			const double titleHeight = MeasureTextHeight(content.risks[index].title, titleStyle, columnWidth, m_measurer);

			EmitTextBlock(
				target, content.risks[index].title, titleStyle,
				Rect{left, risksArea.top, columnWidth, titleHeight}, m_measurer);

			EmitTextBlock(
				target, content.risks[index].body, bodyStyle,
				DropTop(Rect{left, risksArea.top, columnWidth, risksArea.height}, titleHeight + RiskTitleToBodyGap),
				m_measurer);
		}
	}

	if (hasNextStep)
	{
		RectCommand panel;
		panel.bounds = nextStepArea;
		panel.fill = m_theme.palette.ink;
		panel.cornerRadius = m_theme.metrics.card.cornerRadius;
		target.AddRect(panel);

		TextStyle style = nextStepStyle;
		style.align = TextAlign::Left;

		EmitSingleLine(target, content.nextStep, style, InsetRect(nextStepArea, NextStepPadding), m_measurer);
	}
}

double OnePagerLayout::MeasureRequiredHeight(const OnePagerSlideContent& content) const
{
	DrawList scratch;
	const Rect heroArea = Rect{
		m_theme.metrics.page.marginLeft,
		m_theme.metrics.page.marginTop,
		ContentWidth(m_theme.metrics),
		MeasurementScratchHeight};

	const Rect body = PaintHero(scratch, content, heroArea);
	const double rowWidth = body.width;

	const double row2Height = MeasureKpiAndValuationRowHeight(content, rowWidth);
	const double row3Height = MeasureChartBusinessThesisRowHeight(content, rowWidth);
	const double row4Height = MeasureRisksAndNextStepRowHeight(content, rowWidth);

	const double contentBottom = body.top + row2Height + RowGap + row3Height + RowGap + row4Height;

	return contentBottom + SOURCE_NOTE_BOTTOM;
}

DrawList OnePagerLayout::Paint(const OnePagerSlideContent& content, const int pageNumber) const
{
	DrawList result;
	EmitBackground(result, m_theme, m_theme.palette.background);

	const Rect heroArea = Rect{
		m_theme.metrics.page.marginLeft,
		m_theme.metrics.page.marginTop,
		ContentWidth(m_theme.metrics),
		MeasurementScratchHeight};

	const Rect body = PaintHero(result, content, heroArea);
	const double rowWidth = body.width;

	const double row2Height = MeasureKpiAndValuationRowHeight(content, rowWidth);
	const double row3Height = MeasureChartBusinessThesisRowHeight(content, rowWidth);
	const double row4Height = MeasureRisksAndNextStepRowHeight(content, rowWidth);

	const Rect row2 = Rect{body.left, body.top, body.width, row2Height};
	const Rect row3 = Rect{body.left, RectBottom(row2) + RowGap, body.width, row3Height};
	const Rect row4 = Rect{body.left, RectBottom(row3) + RowGap, body.width, row4Height};

	PaintKpiAndValuationRow(result, content, row2);
	PaintChartBusinessThesisRow(result, content, row3);
	PaintRisksAndNextStepRow(result, content, row4);

	EmitSourceNote(result, content.sourceNote, m_theme, m_measurer);
	EmitFooter(result, m_theme, pageNumber, false, m_measurer);

	return result;
}
