#include "StrategyPartnersTheme.hpp"
#include <stdexcept>

namespace
{
void AssertIsExistingDirectory(const std::filesystem::path& root)
{
	if (!std::filesystem::is_directory(root))
	{
		throw std::runtime_error("Каталог с ресурсами оформления не найден");
	}
}

Palette CreatePalette()
{
	Palette palette;
	palette.ink = ColorFromHex("171423");
	palette.text = ColorFromHex("000000");
	palette.textInverse = ColorFromHex("FFFFFF");
	palette.surface = ColorFromHex("F2F3F7");
	palette.background = ColorFromHex("FFFFFF");
	palette.accent = ColorFromHex("02CAD4");
	palette.accentSoft = ColorFromHex("9AF7FF");
	palette.link = ColorFromHex("2386E9");
	palette.violet = ColorFromHex("8E93FE");
	palette.danger = ColorFromHex("E5484D");

	return palette;
}

GradientRamp CreateGradient()
{
	GradientRamp gradient;
	gradient.stops = {
		ColorFromHex("2EEAEF"),
		ColorFromHex("2EE5EE"),
		ColorFromHex("2ED9EE"),
		ColorFromHex("2EC4EE"),
		ColorFromHex("2EA7ED"),
		ColorFromHex("2F8AED")};

	return gradient;
}

TypeScale CreateTypeScale()
{
	TypeScale type;
	type.slideTitle = 32.0;
	type.cardTitle = 12.0;
	type.body = 10.0;
	type.caption = 8.0;
	type.indexNumber = 32.0;
	type.heroNumber = 54.0;
	type.lineHeightFactor = 1.35;
	type.paragraphSpacing = 6.0;

	return type;
}

PageMetrics CreatePageMetrics()
{
	PageMetrics page;
	page.width = 960.0;
	page.height = 540.0;
	page.marginLeft = 34.5;
	page.marginRight = 34.5;
	page.marginTop = 19.0;
	page.marginBottom = 26.0;

	return page;
}

CardMetrics CreateCardMetrics()
{
	CardMetrics card;
	card.cornerRadius = 8.7;
	card.padding = 17.3;
	card.columnGap = 15.1;
	card.rowGap = 15.1;
	card.titleToBodyGap = 24.0;

	return card;
}

BandMetrics CreateBandMetrics()
{
	BandMetrics band;
	band.height = 59.8;
	band.topOffset = 136.8;

	return band;
}

Metrics CreateMetrics()
{
	Metrics metrics;
	metrics.page = CreatePageMetrics();
	metrics.card = CreateCardMetrics();
	metrics.band = CreateBandMetrics();
	metrics.titleBlockTop = 19.0;
	metrics.titleBlockHeight = 99.4;
	metrics.contentTop = 134.6;

	return metrics;
}

std::map<FontRole, std::filesystem::path> CreateFontMap(const std::filesystem::path& root)
{
	const std::filesystem::path fontsRoot = root / "fonts";

	return {
		{FontRole::Regular, fontsRoot / "Inter-Regular.ttf"},
		{FontRole::Bold, fontsRoot / "Inter-Bold.ttf"},
		{FontRole::Light, fontsRoot / "Inter-Light.ttf"}};
}

std::map<AssetRole, std::filesystem::path> CreateAssetMap(const std::filesystem::path& root)
{
	const std::filesystem::path imagesRoot = root / "images";

	return {
		{AssetRole::Logo, imagesRoot / "logo.png"},
		{AssetRole::LogoInverse, imagesRoot / "logo-inverse.png"},
		{AssetRole::TitleDecoration, imagesRoot / "title-decoration.png"},
		{AssetRole::SideDecoration, imagesRoot / "side-decoration.png"},
		{AssetRole::FooterDecoration, imagesRoot / "footer-decoration.png"}};
}
} // namespace

Theme CreateStrategyPartnersTheme(const std::filesystem::path& assetsRoot)
{
	AssertIsExistingDirectory(assetsRoot);

	Theme theme;
	theme.palette = CreatePalette();
	theme.gradient = CreateGradient();
	theme.type = CreateTypeScale();
	theme.metrics = CreateMetrics();
	theme.fonts = CreateFontMap(assetsRoot);
	theme.assets = CreateAssetMap(assetsRoot);

	return theme;
}