#include "PdfGenerator.hpp"
#include "PdfDocument.hpp"
#include "PdfRenderer.hpp"
#include "common/output/pdf/layout/HaruTextMeasurer.hpp"
#include "common/output/pdf/layout/SlideLayoutEngine.hpp"
#include <hpdf.h>
#include <map>

namespace
{
std::map<FontRole, HPDF_Font> LoadThemeFonts(
	const PdfDocument& document,
	const std::map<FontRole, std::filesystem::path>& fontPaths)
{
	std::map<FontRole, HPDF_Font> loadedFonts;

	for (const auto& [role, path] : fontPaths)
	{
		loadedFonts[role] = document.LoadFont(path);
	}

	return loadedFonts;
}

void SetupPageDimensions(const HPDF_Page page, const PageMetrics& metrics)
{
	HPDF_Page_SetWidth(page, static_cast<HPDF_REAL>(metrics.width));
	HPDF_Page_SetHeight(page, static_cast<HPDF_REAL>(metrics.height));
}
} // namespace

void PdfGenerator::Generate(const Deck& deck, const Theme& theme, const std::filesystem::path& outputPath)
{
	PdfDocument document;

	const std::map<FontRole, HPDF_Font> fonts = LoadThemeFonts(document, theme.fonts);
	const HaruTextMeasurer measurer(fonts);
	const SlideLayoutEngine engine(theme, measurer);

	const std::vector<DrawList> pages = engine.BuildDeck(deck);
	PdfRenderer renderer(document, fonts);

	for (const DrawList& list : pages)
	{
		const HPDF_Page page = HPDF_AddPage(document.GetHandle());
		SetupPageDimensions(page, theme.metrics.page);

		renderer.RenderPage(page, theme.metrics.page.height, list);
	}

	document.Save(outputPath);
}