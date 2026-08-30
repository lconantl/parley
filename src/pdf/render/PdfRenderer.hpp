#pragma once

#include "PdfDocument.hpp"
#include "pdf/graphics/DrawList.hpp"
#include "pdf/theme/Theme.hpp"
#include <hpdf.h>
#include <map>
#include <string>

class PdfRenderer
{
public:
	PdfRenderer(PdfDocument& document, std::map<FontRole, HPDF_Font> fonts);

	void RenderPage(HPDF_Page page, double pageHeight, const DrawList& drawList);

private:
	HPDF_Image GetImage(const std::filesystem::path& path);

	PdfDocument& m_document;
	std::map<FontRole, HPDF_Font> m_fonts;
	std::map<std::string, HPDF_Image> m_imageCache;
};