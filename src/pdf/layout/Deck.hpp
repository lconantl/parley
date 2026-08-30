#pragma once

#include "Slide.hpp"
#include <string>
#include <vector>

struct Deck
{
    std::string title;
    std::string author;
    std::vector<Slide> slides;
};
