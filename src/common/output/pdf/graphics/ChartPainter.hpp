#pragma once

#include "common/output/pdf/graphics/DrawList.hpp"
#include "common/output/pdf/layout/ITextMeasurer.hpp"
#include "common/output/pdf/layout/TextBlock.hpp"
#include "common/output/pdf/model/Geometry.hpp"
#include "common/output/pdf/model/Slide.hpp"
#include "common/output/pdf/theme/Theme.hpp"

class ChartPainter
{
public:
	ChartPainter(const Theme& theme, const ITextMeasurer& measurer);

	void Paint(DrawList& target, const ChartSlideContent& content, const Rect& area) const;

private:
	struct ValueRange
	{
		double minimum = 0.0;
		double maximum = 0.0;
	};

	ValueRange MeasureRangeOf(const std::vector<std::vector<double>>& valueSets) const;
	ValueRange MeasureRange(const ChartSlideContent& content) const;
	ValueRange MeasureSecondaryRange(const SecondaryAxisSeries& series) const;
	Rect PlotArea(const Rect& area, bool hasSecondaryAxis) const;

	double ProjectValue(double value, const ValueRange& range, const Rect& plot) const;

	void PaintGrid(DrawList& target, const ValueRange& range, const Rect& plot, const std::string& suffix) const;
	void PaintSecondaryAxisLabels(
		DrawList& target,
		const ValueRange& secondaryRange,
		const Rect& plot,
		const std::string& suffix) const;
	void PaintCategories(DrawList& target, const ChartSlideContent& content, const Rect& plot) const;
	void PaintBars(DrawList& target, const ChartSlideContent& content, const ValueRange& range, const Rect& plot) const;
	void PaintLineSeries(
		DrawList& target,
		const std::vector<double>& values,
		const Color& color,
		const ValueRange& range,
		const Rect& plot) const;
	void PaintLine(DrawList& target, const ChartSlideContent& content, const ValueRange& range, const Rect& plot) const;
	void PaintSecondaryOverlay(
		DrawList& target,
		const SecondaryAxisSeries& series,
		const ValueRange& secondaryRange,
		const Rect& plot,
		std::size_t colorIndex) const;
	void PaintLegend(DrawList& target, const ChartSlideContent& content, const Rect& area) const;

	Color SeriesColor(std::size_t index) const;
	TextStyle LabelStyle() const;

	const Theme& m_theme;
	const ITextMeasurer& m_measurer;
};