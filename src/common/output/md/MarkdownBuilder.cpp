#include "MarkdownBuilder.hpp"

#include <stdexcept>

namespace
{
void AssertIsHeaderLevelValid(const int level)
{
	if (level < 1 || level > 6)
	{
		throw std::invalid_argument("Уровень заголовка должен быть от 1 до 6");
	}
}

void AssertIsNotEmpty(const std::string& text)
{
	if (text.empty())
	{
		throw std::invalid_argument("Текст не должен быть пустым");
	}
}

void AssertIsTableValid(
	const std::vector<std::string>& headers,
	const std::vector<std::vector<std::string>>& rows)
{
	if (headers.empty())
	{
		throw std::invalid_argument("Таблица должна иметь хотя бы один столбец");
	}

	for (const auto& row : rows)
	{
		if (row.size() != headers.size())
		{
			throw std::invalid_argument("Число ячеек в строке не совпадает с числом столбцов");
		}
	}
}

std::string EscapeCell(const std::string& text)
{
	std::string escaped;

	for (const char symbol : text)
	{
		if (symbol == '|')
		{
			escaped += '\\';
		}

		escaped += symbol;
	}

	return escaped.empty() ? " " : escaped;
}

std::string CreateTableRow(const std::vector<std::string>& cells)
{
	std::string row = "|";

	for (const auto& cell : cells)
	{
		row += " " + EscapeCell(cell) + " |";
	}

	return row + "\n";
}

std::string CreateHeaderPrefix(const int level)
{
	return std::string(level, '#') + " ";
}
} // namespace

MarkdownBuilder::MarkdownBuilder()
{
}

MarkdownBuilder::~MarkdownBuilder()
{
}

MarkdownBuilder& MarkdownBuilder::AddHeader(const std::string& text, const int level)
{
	AssertIsHeaderLevelValid(level);
	AssertIsNotEmpty(text);

	m_content += CreateHeaderPrefix(level) + text + "\n\n";
	return *this;
}

MarkdownBuilder& MarkdownBuilder::AddParagraph(const std::string& text)
{
	AssertIsNotEmpty(text);
	m_content += text + "\n\n";
	return *this;
}

MarkdownBuilder& MarkdownBuilder::AddText(const std::string& text)
{
	AssertIsNotEmpty(text);
	m_content += text;
	return *this;
}

MarkdownBuilder& MarkdownBuilder::AddNewLine()
{
	m_content += "\n";
	return *this;
}

MarkdownBuilder& MarkdownBuilder::AddQuote(const std::string& text)
{
	AssertIsNotEmpty(text);
	m_content += "> " + text + "\n\n";
	return *this;
}

MarkdownBuilder& MarkdownBuilder::AddCodeBlock(const std::string& code, const std::string& language)
{
	AssertIsNotEmpty(code);
	m_content += "```" + language + "\n" + code + "\n```\n\n";
	return *this;
}

MarkdownBuilder& MarkdownBuilder::AddUnorderedList(const std::vector<std::string>& items)
{
	for (const auto& item : items)
	{
		AssertIsNotEmpty(item);
		m_content += "- " + item + "\n";
	}
	m_content += "\n";
	return *this;
}

MarkdownBuilder& MarkdownBuilder::AddOrderedList(const std::vector<std::string>& items)
{
	int index = 1;
	for (const auto& item : items)
	{
		AssertIsNotEmpty(item);
		m_content += std::to_string(index++) + ". " + item + "\n";
	}
	m_content += "\n";
	return *this;
}

MarkdownBuilder& MarkdownBuilder::AddTaskList(const std::vector<std::pair<std::string, bool>>& items)
{
	for (const auto& [first, second] : items)
	{
		AssertIsNotEmpty(first);
		std::string check = second ? "[x]" : "[ ]";
		m_content += "- " + check + " " + first + "\n";
	}
	m_content += "\n";
	return *this;
}

MarkdownBuilder& MarkdownBuilder::AddTable(
	const std::vector<std::string>& headers,
	const std::vector<std::vector<std::string>>& rows)
{
	AssertIsTableValid(headers, rows);

	m_content += CreateTableRow(headers);

	std::vector<std::string> separators(headers.size(), "---");
	m_content += CreateTableRow(separators);

	for (const auto& row : rows)
	{
		m_content += CreateTableRow(row);
	}

	m_content += "\n";
	return *this;
}

MarkdownBuilder& MarkdownBuilder::AddHorizontalRule()
{
	m_content += "---\n\n";
	return *this;
}

std::string MarkdownBuilder::MakeBold(const std::string& text)
{
	AssertIsNotEmpty(text);
	return "**" + text + "**";
}

std::string MarkdownBuilder::MakeItalic(const std::string& text)
{
	AssertIsNotEmpty(text);
	return "*" + text + "*";
}

std::string MarkdownBuilder::MakeStrikethrough(const std::string& text)
{
	AssertIsNotEmpty(text);
	return "~~" + text + "~~";
}

std::string MarkdownBuilder::MakeInlineCode(const std::string& text)
{
	AssertIsNotEmpty(text);
	return "`" + text + "`";
}

std::string MarkdownBuilder::MakeLink(const std::string& text, const std::string& url)
{
	AssertIsNotEmpty(text);
	AssertIsNotEmpty(url);
	return "[" + text + "](" + url + ")";
}

std::string MarkdownBuilder::MakeImage(const std::string& altText, const std::string& url)
{
	AssertIsNotEmpty(url);
	return "![" + altText + "](" + url + ")";
}

std::string MarkdownBuilder::Build() const
{
	return m_content;
}