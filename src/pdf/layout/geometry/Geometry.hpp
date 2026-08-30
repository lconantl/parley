#pragma once

struct Point
{
    double x;
    double y;
};

struct Size
{
    double width;
    double height;
};

struct Rect
{
    double left;
    double top;
    double width;
    double height;
};

double RectRight(const Rect& rect);
double RectBottom(const Rect& rect);
Point RectTopLeft(const Rect& rect);
Rect InsetRect(const Rect& rect, double inset);
Rect TakeTop(const Rect& rect, double height);
Rect DropTop(const Rect& rect, double height);
