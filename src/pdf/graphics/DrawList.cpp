#include "DrawList.hpp"

#include "DrawCommand.hpp"

void DrawList::AddRect(const RectCommand& command)
{
	m_commands.emplace_back(command);
}

void DrawList::AddLine(const LineCommand& command)
{
	m_commands.emplace_back(command);
}

void DrawList::AddText(const TextCommand& command)
{
	m_commands.emplace_back(command);
}

void DrawList::AddImage(const ImageCommand& command)
{
	m_commands.emplace_back(command);
}

void DrawList::Append(const DrawList& other)
{
	m_commands.insert(m_commands.end(), other.m_commands.begin(), other.m_commands.end());
}

const std::vector<DrawCommand>& DrawList::Commands() const noexcept
{
	return m_commands;
}

bool DrawList::IsEmpty() const noexcept
{
	return m_commands.empty();
}
