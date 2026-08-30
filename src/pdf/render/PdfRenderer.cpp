#include "PdfRenderer.hpp"
#include "pdf/layout/DrawCommand.hpp"
#include <stdexcept>

namespace
{
template <typename... Handlers>
struct Overloaded : Handlers...
{
	using Handlers::operator()...;
};

void AssertIsKnownFont(const std::map<FontRole, HPDF_Font>& fonts, FontRole role)
{
	if (!fonts.contains(role))
	{
		throw std::invalid_argument("Неизвестная роль шрифта при рендеринге текста");
	}
}

double CalculateInvertedY(const double top, const double height, const double pageHeight)
{
	return pageHeight - top - height;
}

double CalculateBaselineY(const double top, const double fontSize, const double pageHeight)
{
	return pageHeight - top - fontSize;
}

void ApplyFillColor(const HPDF_Page page, const Color& color)
{
	HPDF_Page_SetRGBFill(
		page,
		static_cast<HPDF_REAL>(color.red),
		static_cast<HPDF_REAL>(color.green),
		static_cast<HPDF_REAL>(color.blue));
}

void ApplyStrokeColor(const HPDF_Page page, const Color& color)
{
	HPDF_Page_SetRGBStroke(
		page,
		static_cast<HPDF_REAL>(color.red),
		static_cast<HPDF_REAL>(color.green),
		static_cast<HPDF_REAL>(color.blue));
}

void RenderRect(const HPDF_Page page, const double pageHeight, const RectCommand& command)
{
	ApplyFillColor(page, command.fill);

	const double pdfY = CalculateInvertedY(command.bounds.top, command.bounds.height, pageHeight);

	HPDF_Page_Rectangle(
		page,
		static_cast<HPDF_REAL>(command.bounds.left),
		static_cast<HPDF_REAL>(pdfY),
		static_cast<HPDF_REAL>(command.bounds.width),
		static_cast<HPDF_REAL>(command.bounds.height));

	HPDF_Page_Fill(page);
}

void RenderLine(const HPDF_Page page, const double pageHeight, const LineCommand& command)
{
	ApplyStrokeColor(page, command.stroke);
	HPDF_Page_SetLineWidth(page, static_cast<HPDF_REAL>(command.thickness));

	const double startY = pageHeight - command.from.y;
	const double endY = pageHeight - command.to.y;

	HPDF_Page_MoveTo(page, static_cast<HPDF_REAL>(command.from.x), static_cast<HPDF_REAL>(startY));
	HPDF_Page_LineTo(page, static_cast<HPDF_REAL>(command.to.x), static_cast<HPDF_REAL>(endY));
	HPDF_Page_Stroke(page);
}

void RenderText(
	const HPDF_Page page,
	const double pageHeight,
	const TextCommand& command,
	const std::map<FontRole, HPDF_Font>& fonts)
{
	AssertIsKnownFont(fonts, command.font);

	ApplyFillColor(page, command.color);
	HPDF_Page_SetFontAndSize(page, fonts.at(command.font), static_cast<HPDF_REAL>(command.fontSize));

	const double baselineY = CalculateBaselineY(command.lineBoxTopLeft.y, command.fontSize, pageHeight);

	HPDF_Page_BeginText(page);
	HPDF_Page_MoveTextPos(page, static_cast<HPDF_REAL>(command.lineBoxTopLeft.x), static_cast<HPDF_REAL>(baselineY));
	HPDF_Page_ShowText(page, command.text.c_str());
	HPDF_Page_EndText(page);
}

void RenderImage(const HPDF_Page page, const double pageHeight, const ImageCommand& command, const HPDF_Image image)
{
	const double pdfY = CalculateInvertedY(command.bounds.top, command.bounds.height, pageHeight);

	HPDF_Page_DrawImage(
		page,
		image,
		static_cast<HPDF_REAL>(command.bounds.left),
		static_cast<HPDF_REAL>(pdfY),
		static_cast<HPDF_REAL>(command.bounds.width),
		static_cast<HPDF_REAL>(command.bounds.height));
}
} // namespace

PdfRenderer::PdfRenderer(PdfDocument& document, std::map<FontRole, HPDF_Font> fonts)
	: m_document(document)
	, m_fonts(std::move(fonts))
{
}

void PdfRenderer::RenderPage(const HPDF_Page page, const double pageHeight, const DrawList& drawList)
{
	for (const DrawCommand& command : drawList.Commands())
	{
		std::visit(
			Overloaded{
				[&](const RectCommand& cmd) { RenderRect(page, pageHeight, cmd); },
				[&](const LineCommand& cmd) { RenderLine(page, pageHeight, cmd); },
				[&](const TextCommand& cmd) { RenderText(page, pageHeight, cmd, m_fonts); },
				[&](const ImageCommand& cmd) { RenderImage(page, pageHeight, cmd, GetImage(cmd.source)); }},
			command);
	}
}

HPDF_Image PdfRenderer::GetImage(const std::filesystem::path& path)
{
	const std::string key = path.string();

	if (!m_imageCache.contains(key))
	{
		m_imageCache[key] = m_document.LoadImage(path);
	}

	return m_imageCache.at(key);
}