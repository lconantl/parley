#pragma once

#include "ITextMeasurer.hpp"
#include "common/output/pdf/graphics/DrawList.hpp"
#include "common/output/pdf/model/Geometry.hpp"
#include "common/output/pdf/model/Slide.hpp"
#include "common/output/pdf/theme/Theme.hpp"
#include "common/output/pdf/theme/TextStyles.hpp"

class OnePagerLayout
{
public:
	OnePagerLayout(const Theme& theme, const ITextMeasurer& measurer);

	DrawList Paint(const OnePagerSlideContent& content, int pageNumber) const;

	// Computes the page height this content needs, using the same real font-metric
	// measurements as Paint(), so the caller can size the page before rendering it.
	double MeasureRequiredHeight(const OnePagerSlideContent& content) const;

private:
	Rect PaintHero(DrawList& target, const OnePagerSlideContent& content, const Rect& area) const;

	double MeasureKpiAndValuationRowHeight(const OnePagerSlideContent& content, double rowWidth) const;
	void PaintKpiAndValuationRow(DrawList& target, const OnePagerSlideContent& content, const Rect& area) const;

	TextStyle MeasureKpiValueStyle(const std::vector<KpiEntry>& kpis, double tileWidth) const;
	double MeasureKpiTileHeight(const std::vector<KpiEntry>& kpis, const TextStyle& valueStyle) const;

	double MeasureValuationBridgeHeight(const std::vector<ValuationBridgeStep>& steps, const std::string& note) const;
	void PaintValuationBridge(
		DrawList& target,
		const std::vector<ValuationBridgeStep>& steps,
		const std::string& note,
		const Rect& area) const;

	double MeasureRevenueMixHeight(const std::vector<RevenueMixSegment>& segments, double areaWidth) const;
	void PaintRevenueMix(DrawList& target, const std::vector<RevenueMixSegment>& segments, const Rect& area) const;

	double MeasureChartBusinessThesisRowHeight(const OnePagerSlideContent& content, double rowWidth) const;
	void PaintChartBusinessThesisRow(DrawList& target, const OnePagerSlideContent& content, const Rect& area) const;

	double MeasureRisksAndNextStepRowHeight(const OnePagerSlideContent& content, double rowWidth) const;
	void PaintRisksAndNextStepRow(DrawList& target, const OnePagerSlideContent& content, const Rect& area) const;

	const Theme& m_theme;
	const ITextMeasurer& m_measurer;
};
