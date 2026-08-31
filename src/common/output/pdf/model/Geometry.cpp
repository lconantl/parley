#include "Geometry.hpp"

#include <algorithm>
#include <stdexcept>

namespace
{
    void AssertIsNonNegative(const double value)
    {
        if (value < 0.0)
        {
            throw std::invalid_argument("Размер области не может быть отрицательным");
        }
    }
}

double RectRight(const Rect& rect)
{
    return rect.left + rect.width;
}

double RectBottom(const Rect& rect)
{
    return rect.top + rect.height;
}

Point RectTopLeft(const Rect& rect)
{
    return Point{rect.left, rect.top};
}

Rect InsetRect(const Rect& rect, const double inset)
{
    AssertIsNonNegative(inset);

    return Rect{
        rect.left + inset,
        rect.top + inset,
        std::max(0.0, rect.width - inset * 2.0),
        std::max(0.0, rect.height - inset * 2.0)};
}

Rect TakeTop(const Rect& rect, const double height)
{
    AssertIsNonNegative(height);

    return Rect{rect.left, rect.top, rect.width, std::min(height, rect.height)};
}

Rect DropTop(const Rect& rect, const double height)
{
    AssertIsNonNegative(height);

    const double consumed = std::min(height, rect.height);

    return Rect{rect.left, rect.top + consumed, rect.width, rect.height - consumed};
}
