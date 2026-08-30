#pragma once

#include "deck/Deck.hpp"
#include "drawlist/DrawList.hpp"

#include <vector>

class ISlideLayoutEngine
{
public:
    virtual ~ISlideLayoutEngine() = default;

    virtual DrawList BuildSlide(const Slide& slide, int pageNumber) const = 0;
    virtual std::vector<DrawList> BuildDeck(const Deck& deck) const = 0;
};
