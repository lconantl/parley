#pragma once

#include <string>
#include <vector>

class MarkdownBuilder
{
public:
	MarkdownBuilder();
	~MarkdownBuilder();

	MarkdownBuilder& AddHeader(const std::string& text, int level);
	MarkdownBuilder& AddParagraph(const std::string& text);
	MarkdownBuilder& AddText(const std::string& text);
	MarkdownBuilder& AddNewLine();
	MarkdownBuilder& AddQuote(const std::string& text);
	MarkdownBuilder& AddCodeBlock(const std::string& code, const std::string& language);
	MarkdownBuilder& AddUnorderedList(const std::vector<std::string>& items);
	MarkdownBuilder& AddOrderedList(const std::vector<std::string>& items);
	MarkdownBuilder& AddTaskList(const std::vector<std::pair<std::string, bool>>& items);
	MarkdownBuilder& AddTable(
		const std::vector<std::string>& headers,
		const std::vector<std::vector<std::string>>& rows);
	MarkdownBuilder& AddHorizontalRule();

	static std::string MakeBold(const std::string& text);
	static std::string MakeItalic(const std::string& text);
	static std::string MakeStrikethrough(const std::string& text);
	static std::string MakeInlineCode(const std::string& text);
	static std::string MakeLink(const std::string& text, const std::string& url);
	static std::string MakeImage(const std::string& altText, const std::string& url);

	std::string Build() const;

private:
	std::string m_content;
};