#include "ChartPainter.hpp"

#include "common/output/pdf/layout/TextBlock.hpp"
#include "common/output/pdf/theme/TextStyles.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
constexpr double AxisLabelWidth = 74.0;
constexpr double CategoryLabelHeight = 26.0;
constexpr double LegendHeight = 22.0;
constexpr double LegendMarkerSize = 9.0;
constexpr double LegendGap = 10.0;
constexpr double LegendItemGap = 26.0;
constexpr double GridThickness = 0.5;
constexpr double AxisThickness = 1.0;
constexpr double LineThickness = 2.2;
constexpr double PointSize = 5.0;
constexpr double BarGroupGap = 0.34;
constexpr double BarInnerGap = 4.0;
constexpr double ValueLabelOffset = 14.0;
constexpr int GridSteps = 4;
constexpr double Epsilon = 1e-9;

void AssertIsChartUsable(const ChartSlideContent& content)
{
	if (content.series.empty() || content.categories.empty())
	{
		throw std::invalid_argument("График должен содержать категории и хотя бы один ряд");
	}

	for (const auto& series : content.series)
	{
		if (series.values.size() != content.categories.size())
		{
			throw std::invalid_argument("Число значений в ряду не совпадает с числом категорий");
		}
	}

	if (content.secondarySeries.has_value()
		&& content.secondarySeries->values.size() != content.categories.size())
	{
		throw std::invalid_argument("Число значений во вторичном ряду не совпадает с числом категорий");
	}
}

std::string FormatAxisValue(const double value, const std::string& suffix)
{
	const double magnitude = std::abs(value);
	double scaled = value;
	std::string unit;

	if (suffix.empty())
	{
		if (magnitude >= 1000000000.0)
		{
			scaled = value / 1000000000.0;
			unit = " млрд";
		}
		else if (magnitude >= 1000000.0)
		{
			scaled = value / 1000000.0;
			unit = " млн";
		}
		else if (magnitude >= 1000.0)
		{
			scaled = value / 1000.0;
			unit = " тыс";
		}
	}
	else
	{
		unit = suffix;
	}

	std::ostringstream stream;
	stream.precision(std::abs(scaled) < 10.0 ? 1 : 0);
	stream << std::fixed << scaled;

	std::string text = stream.str();
	const std::size_t separator = text.find('.');
	if (separator != std::string::npos)
	{
		text[separator] = ',';
	}

	return text + unit;
}

int LabelStride(const double plotHeight, const double lineHeight, const int totalSteps)
{
	if (lineHeight <= 0.0)
	{
		return 1;
	}

	const int maxLabels = std::max(2, static_cast<int>(std::floor(plotHeight / lineHeight)) + 1);

	if (maxLabels >= totalSteps + 1)
	{
		return 1;
	}

	return static_cast<int>(std::ceil(static_cast<double>(totalSteps + 1) / static_cast<double>(maxLabels)));
}

double NiceStep(const double span)
{
	if (span < Epsilon)
	{
		return 1.0;
	}

	const double rough = span / GridSteps;
	const double exponent = std::floor(std::log10(rough));
	const double base = std::pow(10.0, exponent);
	const double normalized = rough / base;

	if (normalized <= 1.0)
	{
		return base;
	}

	if (normalized <= 2.0)
	{
		return 2.0 * base;
	}

	if (normalized <= 5.0)
	{
		return 5.0 * base;
	}

	return 10.0 * base;
}
} // namespace

ChartPainter::ChartPainter(const Theme& theme, const ITextMeasurer& measurer)
	: m_theme(theme)
	, m_measurer(measurer)
{
}

Color ChartPainter::SeriesColor(const std::size_t index) const
{
	const std::vector<Color> palette{
		m_theme.palette.accent,
		m_theme.palette.ink,
		m_theme.palette.violet,
		m_theme.palette.link,
		m_theme.palette.accentSoft};

	return palette[index % palette.size()];
}

TextStyle ChartPainter::LabelStyle() const
{
	TextStyle style = CaptionStyle(m_theme);
	style.color = m_theme.palette.text;

	return style;
}

ChartPainter::ValueRange ChartPainter::MeasureRangeOf(const std::vector<std::vector<double>>& valueSets) const
{
	ValueRange range;

	for (const auto& values : valueSets)
	{
		for (const double value : values)
		{
			range.minimum = std::min(range.minimum, value);
			range.maximum = std::max(range.maximum, value);
		}
	}

	if (std::abs(range.maximum - range.minimum) < Epsilon)
	{
		range.maximum = range.minimum + 1.0;
	}

	const double step = NiceStep(range.maximum - range.minimum);
	range.maximum = std::ceil(range.maximum / step) * step;
	range.minimum = std::floor(range.minimum / step) * step;

	return range;
}

ChartPainter::ValueRange ChartPainter::MeasureRange(const ChartSlideContent& content) const
{
	std::vector<std::vector<double>> valueSets;
	valueSets.reserve(content.series.size());

	for (const auto& series : content.series)
	{
		valueSets.push_back(series.values);
	}

	return MeasureRangeOf(valueSets);
}

ChartPainter::ValueRange ChartPainter::MeasureSecondaryRange(const SecondaryAxisSeries& series) const
{
	return MeasureRangeOf({series.values});
}

Rect ChartPainter::PlotArea(const Rect& area, const bool hasSecondaryAxis) const
{
	const double rightGutter = hasSecondaryAxis ? AxisLabelWidth : 0.0;

	return Rect{
		area.left + AxisLabelWidth,
		area.top + LegendHeight,
		area.width - AxisLabelWidth - rightGutter,
		area.height - LegendHeight - CategoryLabelHeight};
}

double ChartPainter::ProjectValue(
	const double value,
	const ValueRange& range,
	const Rect& plot) const
{
	const double span = range.maximum - range.minimum;
	const double ratio = (value - range.minimum) / span;

	return RectBottom(plot) - ratio * plot.height;
}

void ChartPainter::PaintGrid(
	DrawList& target,
	const ValueRange& range,
	const Rect& plot,
	const std::string& suffix) const
{
	TextStyle style = LabelStyle();
	style.align = TextAlign::Right;

	const double step = (range.maximum - range.minimum) / GridSteps;
	const int stride = LabelStride(plot.height, style.lineHeight, GridSteps);

	for (int index = 0; index <= GridSteps; ++index)
	{
		const double value = range.minimum + step * index;
		const double y = ProjectValue(value, range, plot);

		LineCommand grid;
		grid.from = Point{plot.left, y};
		grid.to = Point{RectRight(plot), y};
		grid.stroke = m_theme.palette.accentSoft;
		grid.thickness = GridThickness;

		target.AddLine(grid);

		if (index % stride != 0 && index != GridSteps)
		{
			continue;
		}

		const Rect labelArea = Rect{
			plot.left - AxisLabelWidth,
			y - style.lineHeight / 2.0,
			AxisLabelWidth - LegendGap,
			style.lineHeight};

		EmitSingleLine(target, FormatAxisValue(value, suffix), style, labelArea, m_measurer);
	}

	const double baseline = ProjectValue(std::max(range.minimum, 0.0), range, plot);

	LineCommand axis;
	axis.from = Point{plot.left, baseline};
	axis.to = Point{RectRight(plot), baseline};
	axis.stroke = m_theme.palette.text;
	axis.thickness = AxisThickness;

	target.AddLine(axis);
}

void ChartPainter::PaintSecondaryAxisLabels(
	DrawList& target,
	const ValueRange& secondaryRange,
	const Rect& plot,
	const std::string& suffix) const
{
	TextStyle style = LabelStyle();
	style.align = TextAlign::Left;

	const double step = (secondaryRange.maximum - secondaryRange.minimum) / GridSteps;
	const int stride = LabelStride(plot.height, style.lineHeight, GridSteps);

	for (int index = 0; index <= GridSteps; ++index)
	{
		if (index % stride != 0 && index != GridSteps)
		{
			continue;
		}

		const double value = secondaryRange.minimum + step * index;
		const double y = ProjectValue(value, secondaryRange, plot);

		const Rect labelArea = Rect{
			RectRight(plot) + LegendGap,
			y - style.lineHeight / 2.0,
			AxisLabelWidth - LegendGap,
			style.lineHeight};

		EmitSingleLine(target, FormatAxisValue(value, suffix), style, labelArea, m_measurer);
	}
}

void ChartPainter::PaintCategories(
	DrawList& target,
	const ChartSlideContent& content,
	const Rect& plot) const
{
	TextStyle style = LabelStyle();
	style.align = TextAlign::Center;

	const double slot = plot.width / static_cast<double>(content.categories.size());

	for (std::size_t index = 0; index < content.categories.size(); ++index)
	{
		const Rect labelArea = Rect{
			plot.left + slot * static_cast<double>(index),
			RectBottom(plot) + LegendGap,
			slot,
			style.lineHeight};

		EmitSingleLine(target, content.categories[index], style, labelArea, m_measurer);
	}
}

void ChartPainter::PaintBars(
	DrawList& target,
	const ChartSlideContent& content,
	const ValueRange& range,
	const Rect& plot) const
{
	const double slot = plot.width / static_cast<double>(content.categories.size());
	const double groupWidth = slot * (1.0 - BarGroupGap);
	const double seriesCount = static_cast<double>(content.series.size());
	const double barWidth = (groupWidth - BarInnerGap * (seriesCount - 1.0)) / seriesCount;
	const double baseline = ProjectValue(std::max(range.minimum, 0.0), range, plot);

	TextStyle valueStyle = LabelStyle();
	valueStyle.align = TextAlign::Center;

	for (std::size_t seriesIndex = 0; seriesIndex < content.series.size(); ++seriesIndex)
	{
		const auto& series = content.series[seriesIndex];

		for (std::size_t index = 0; index < series.values.size(); ++index)
		{
			const double value = series.values[index];
			const double top = ProjectValue(value, range, plot);

			const double left = plot.left
				+ slot * static_cast<double>(index)
				+ (slot - groupWidth) / 2.0
				+ (barWidth + BarInnerGap) * static_cast<double>(seriesIndex);

			RectCommand bar;
			bar.bounds = Rect{left, std::min(top, baseline), barWidth, std::abs(baseline - top)};
			bar.fill = SeriesColor(seriesIndex);
			bar.cornerRadius = 0.0;

			target.AddRect(bar);

			if (content.series.size() > 1)
			{
				continue;
			}

			const Rect labelArea = Rect{
				left - BarInnerGap,
				top - ValueLabelOffset,
				barWidth + BarInnerGap * 2.0,
				valueStyle.lineHeight};

			EmitSingleLine(
				target,
				FormatAxisValue(value, content.valueSuffix),
				valueStyle,
				labelArea,
				m_measurer);
		}
	}
}

void ChartPainter::PaintLineSeries(
	DrawList& target,
	const std::vector<double>& values,
	const Color& color,
	const ValueRange& range,
	const Rect& plot) const
{
	const double slot = plot.width / static_cast<double>(values.size());

	for (std::size_t index = 0; index + 1 < values.size(); ++index)
	{
		LineCommand segment;
		segment.from = Point{
			plot.left + slot * (static_cast<double>(index) + 0.5),
			ProjectValue(values[index], range, plot)};
		segment.to = Point{
			plot.left + slot * (static_cast<double>(index) + 1.5),
			ProjectValue(values[index + 1], range, plot)};
		segment.stroke = color;
		segment.thickness = LineThickness;

		target.AddLine(segment);
	}

	for (std::size_t index = 0; index < values.size(); ++index)
	{
		const double x = plot.left + slot * (static_cast<double>(index) + 0.5);
		const double y = ProjectValue(values[index], range, plot);

		RectCommand point;
		point.bounds = Rect{
			x - PointSize / 2.0,
			y - PointSize / 2.0,
			PointSize,
			PointSize};
		point.fill = color;
		point.cornerRadius = PointSize / 2.0;

		target.AddRect(point);
	}
}

void ChartPainter::PaintLine(
	DrawList& target,
	const ChartSlideContent& content,
	const ValueRange& range,
	const Rect& plot) const
{
	for (std::size_t seriesIndex = 0; seriesIndex < content.series.size(); ++seriesIndex)
	{
		PaintLineSeries(target, content.series[seriesIndex].values, SeriesColor(seriesIndex), range, plot);
	}
}

void ChartPainter::PaintSecondaryOverlay(
	DrawList& target,
	const SecondaryAxisSeries& series,
	const ValueRange& secondaryRange,
	const Rect& plot,
	const std::size_t colorIndex) const
{
	PaintLineSeries(target, series.values, SeriesColor(colorIndex), secondaryRange, plot);
}

void ChartPainter::PaintLegend(
	DrawList& target,
	const ChartSlideContent& content,
	const Rect& area) const
{
	std::vector<std::string> names;
	for (const auto& series : content.series)
	{
		names.push_back(series.name);
	}

	if (content.secondarySeries.has_value())
	{
		names.push_back(content.secondarySeries->name);
	}

	if (names.size() < 2)
	{
		return;
	}

	TextStyle style = LabelStyle();
	style.align = TextAlign::Left;

	double cursor = area.left + AxisLabelWidth;

	for (std::size_t index = 0; index < names.size(); ++index)
	{
		RectCommand marker;
		marker.bounds = Rect{
			cursor,
			area.top + (LegendHeight - LegendMarkerSize) / 2.0,
			LegendMarkerSize,
			LegendMarkerSize};
		marker.fill = SeriesColor(index);
		marker.cornerRadius = 0.0;

		target.AddRect(marker);

		const double labelWidth = m_measurer.MeasureWidth(names[index], style.font, style.fontSize);

		const Rect labelArea = Rect{
			cursor + LegendMarkerSize + LegendGap,
			area.top + (LegendHeight - style.lineHeight) / 2.0,
			labelWidth,
			style.lineHeight};

		EmitSingleLine(target, names[index], style, labelArea, m_measurer);

		cursor += LegendMarkerSize + LegendGap + labelWidth + LegendItemGap;
	}
}

void ChartPainter::Paint(
	DrawList& target,
	const ChartSlideContent& content,
	const Rect& area) const
{
	AssertIsChartUsable(content);

	const bool hasSecondary = content.secondarySeries.has_value();
	const ValueRange range = MeasureRange(content);
	const Rect plot = PlotArea(area, hasSecondary);

	PaintLegend(target, content, area);
	PaintGrid(target, range, plot, content.valueSuffix);
	PaintCategories(target, content, plot);

	if (content.kind == ChartKind::Line)
	{
		PaintLine(target, content, range, plot);
	}
	else
	{
		PaintBars(target, content, range, plot);
	}

	if (hasSecondary)
	{
		const ValueRange secondaryRange = MeasureSecondaryRange(*content.secondarySeries);
		PaintSecondaryAxisLabels(target, secondaryRange, plot, content.secondarySeries->valueSuffix);
		PaintSecondaryOverlay(target, *content.secondarySeries, secondaryRange, plot, content.series.size());
	}
}