#pragma once

#include "drawlist/DrawCommand.hpp"

#include <vector>

class DrawList
{
public:
    void AddRect(const RectCommand& command);
    void AddLine(const LineCommand& command);
    void AddText(const TextCommand& command);
    void AddImage(const ImageCommand& command);
    void Append(const DrawList& other);

    const std::vector<DrawCommand>& Commands() const noexcept;
    bool IsEmpty() const noexcept;

private:
    std::vector<DrawCommand> m_commands;
};
