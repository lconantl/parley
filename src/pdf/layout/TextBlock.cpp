#include "layout/TextBlock.hpp"

#include <stdexcept>

namespace
{
    constexpr char LINE_BREAK = '\n';
    constexpr char WORD_SEPARATOR = ' ';

    void AssertIsPositiveWidth(double maxWidth)
    {
        if (maxWidth <= 0.0)
        {
            throw std::invalid_argument("Ширина области текста должна быть положительной");
        }
    }

    void AssertIsPositiveFontSize(double fontSize)
    {
        if (fontSize <= 0.0)
        {
            throw std::invalid_argument("Кегль шрифта должен быть положительным");
        }
    }

    std::vector<std::string_view> SplitByLineBreaks(std::string_view text)
    {
        std::vector<std::string_view> paragraphs;
        std::size_t start = 0;

        while (start <= text.size())
        {
            const std::size_t found = text.find(LINE_BREAK, start);

            if (found == std::string_view::npos)
            {
                paragraphs.push_back(text.substr(start));
                break;
            }

            paragraphs.push_back(text.substr(start, found - start));
            start = found + 1;
        }

        return paragraphs;
    }

    std::vector<std::string_view> SplitByWords(std::string_view paragraph)
    {
        std::vector<std::string_view> words;
        std::size_t start = 0;

        while (start < paragraph.size())
        {
            const std::size_t found = paragraph.find(WORD_SEPARATOR, start);

            if (found == std::string_view::npos)
            {
                words.push_back(paragraph.substr(start));
                break;
            }

            if (found > start)
            {
                words.push_back(paragraph.substr(start, found - start));
            }

            start = found + 1;
        }

        return words;
    }

    std::string JoinWord(const std::string& line, std::string_view word)
    {
        if (line.empty())
        {
            return std::string(word);
        }

        return line + WORD_SEPARATOR + std::string(word);
    }

    bool FitsIntoWidth(
        const std::string& candidate,
        const TextStyle& style,
        double maxWidth,
        const ITextMeasurer& measurer)
    {
        return measurer.MeasureWidth(candidate, style.font, style.fontSize) <= maxWidth;
    }

    void WrapParagraph(
        std::string_view paragraph,
        const TextStyle& style,
        double maxWidth,
        const ITextMeasurer& measurer,
        std::vector<std::string>& lines)
    {
        const std::vector<std::string_view> words = SplitByWords(paragraph);

        if (words.empty())
        {
            lines.emplace_back();
            return;
        }

        std::string current;

        for (const std::string_view& word : words)
        {
            const std::string candidate = JoinWord(current, word);

            if (current.empty() || FitsIntoWidth(candidate, style, maxWidth, measurer))
            {
                current = candidate;
                continue;
            }

            lines.push_back(current);
            current = std::string(word);
        }

        lines.push_back(current);
    }

    double AlignedLeft(
        const std::string& line,
        const TextStyle& style,
        const Rect& area,
        const ITextMeasurer& measurer)
    {
        if (style.align == TextAlign::Left)
        {
            return area.left;
        }

        const double lineWidth = measurer.MeasureWidth(line, style.font, style.fontSize);

        if (style.align == TextAlign::Right)
        {
            return RectRight(area) - lineWidth;
        }

        return area.left + (area.width - lineWidth) / 2.0;
    }
}

std::vector<std::string> WrapText(
    std::string_view text,
    const TextStyle& style,
    double maxWidth,
    const ITextMeasurer& measurer)
{
    AssertIsPositiveWidth(maxWidth);
    AssertIsPositiveFontSize(style.fontSize);

    std::vector<std::string> lines;

    for (const std::string_view& paragraph : SplitByLineBreaks(text))
    {
        WrapParagraph(paragraph, style, maxWidth, measurer, lines);
    }

    return lines;
}

double MeasureTextHeight(
    std::string_view text,
    const TextStyle& style,
    double maxWidth,
    const ITextMeasurer& measurer)
{
    const std::vector<std::string> lines = WrapText(text, style, maxWidth, measurer);

    return static_cast<double>(lines.size()) * style.lineHeight;
}

double EmitTextBlock(
    DrawList& target,
    std::string_view text,
    const TextStyle& style,
    const Rect& area,
    const ITextMeasurer& measurer)
{
    const std::vector<std::string> lines = WrapText(text, style, area.width, measurer);
    double cursor = area.top;

    for (const std::string& line : lines)
    {
        TextCommand command;
        command.lineBoxTopLeft = Point{AlignedLeft(line, style, area, measurer), cursor};
        command.text = line;
        command.font = style.font;
        command.fontSize = style.fontSize;
        command.lineHeight = style.lineHeight;
        command.color = style.color;

        target.AddText(command);
        cursor += style.lineHeight;
    }

    return cursor - area.top;
}

void EmitSingleLine(
    DrawList& target,
    std::string_view text,
    const TextStyle& style,
    const Rect& area,
    const ITextMeasurer& measurer)
{
    AssertIsPositiveFontSize(style.fontSize);

    const std::string line(text);

    TextCommand command;
    command.lineBoxTopLeft = Point{AlignedLeft(line, style, area, measurer), area.top};
    command.text = line;
    command.font = style.font;
    command.fontSize = style.fontSize;
    command.lineHeight = style.lineHeight;
    command.color = style.color;

    target.AddText(command);
}
