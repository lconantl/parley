#pragma once

#include "ISlideLayoutEngine.hpp"
#include "ITextMeasurer.hpp"
#include "pdf/graphics/DrawList.hpp"
#include "pdf/model/Deck.hpp"
#include "pdf/model/Slide.hpp"
#include "pdf/theme/Theme.hpp"

class SlideLayoutEngine : public ISlideLayoutEngine
{
public:
	SlideLayoutEngine(Theme theme, const ITextMeasurer& measurer);

	DrawList BuildSlide(const Slide& slide, int pageNumber) const override;
	std::vector<DrawList> BuildDeck(const Deck& deck) const override;

private:
	Theme m_theme;
	const ITextMeasurer& m_measurer;
};
