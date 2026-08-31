#include "InvestmentReportTheme.hpp"
#include <stdexcept>

namespace
{
constexpr auto FontsFolder = "fonts";
constexpr auto ImagesFolder = "images";
constexpr auto RegularFontName = "Verdana.ttf";
constexpr auto BoldFontName = "Verdana-Bold.ttf";
constexpr auto LightFontName = "Verdana-Light.ttf";
constexpr auto LogoName = "logo.png";
constexpr auto LogoInverseName = "logo-inverse.png";

void AssertIsExistingDirectory(const std::filesystem::path& root)
{
	if (!std::filesystem::is_directory(root))
	{
		throw std::runtime_error(
			"Каталог с ресурсами оформления не найден: " + root.string());
	}
}

void AssertIsExistingFile(const std::filesystem::path& path)
{
	if (!std::filesystem::is_regular_file(path))
	{
		throw std::runtime_error("Не найден файл шрифта: " + path.string());
	}
}

Palette CreatePalette()
{
	Palette palette;
	palette.ink = ColorFromHex("11213A");
	palette.text = ColorFromHex("1A1A1A");
	palette.textInverse = ColorFromHex("FFFFFF");
	palette.surface = ColorFromHex("F4F6F9");
	palette.background = ColorFromHex("FFFFFF");
	palette.accent = ColorFromHex("1F5FA8");
	palette.accentSoft = ColorFromHex("D6DEE8");
	palette.link = ColorFromHex("1F5FA8");
	palette.violet = ColorFromHex("6E7C91");

	return palette;
}

GradientRamp CreateGradient()
{
	GradientRamp gradient;
	gradient.stops = {
		ColorFromHex("1F5FA8"),
		ColorFromHex("11213A"),
		ColorFromHex("6E7C91"),
		ColorFromHex("A8B4C4"),
		ColorFromHex("D6DEE8")};

	return gradient;
}

TypeScale CreateTypeScale()
{
	TypeScale type;
	type.slideTitle = 27.0;
	type.cardTitle = 13.0;
	type.body = 10.5;
	type.caption = 8.0;
	type.indexNumber = 22.0;
	type.heroNumber = 44.0;
	type.lineHeightFactor = 1.42;
	type.paragraphSpacing = 9.0;

	return type;
}

PageMetrics CreatePageMetrics()
{
	PageMetrics page;
	page.width = 960.0;
	page.height = 540.0;
	page.marginLeft = 56.0;
	page.marginRight = 56.0;
	page.marginTop = 44.0;
	page.marginBottom = 40.0;

	return page;
}

CardMetrics CreateCardMetrics()
{
	CardMetrics card;
	card.cornerRadius = 2.0;
	card.padding = 18.0;
	card.columnGap = 20.0;
	card.rowGap = 18.0;
	card.titleToBodyGap = 10.0;

	return card;
}

Metrics CreateMetrics()
{
	Metrics metrics;
	metrics.page = CreatePageMetrics();
	metrics.card = CreateCardMetrics();
	metrics.band.height = 58.0;
	metrics.band.topOffset = 128.0;
	metrics.titleBlockTop = 46.0;
	metrics.titleBlockHeight = 44.0;
	metrics.contentTop = 122.0;

	return metrics;
}

std::map<FontRole, std::filesystem::path> CreateFontMap(const std::filesystem::path& root)
{
	const std::filesystem::path fontsRoot = root / FontsFolder;

	const std::filesystem::path regular = fontsRoot / RegularFontName;
	const std::filesystem::path bold = fontsRoot / BoldFontName;
	const std::filesystem::path light = fontsRoot / LightFontName;

	AssertIsExistingFile(regular);
	AssertIsExistingFile(bold);

	return {
		{FontRole::Regular, regular},
		{FontRole::Bold, bold},
		{FontRole::Light, std::filesystem::is_regular_file(light) ? light : regular}};
}

void AddOptionalAsset(
	std::map<AssetRole, std::filesystem::path>& assets,
	const AssetRole role,
	const std::filesystem::path& path)
{
	if (!std::filesystem::is_regular_file(path))
	{
		return;
	}

	assets[role] = path;
}

std::map<AssetRole, std::filesystem::path> CreateAssetMap(const std::filesystem::path& root)
{
	const std::filesystem::path imagesRoot = root / ImagesFolder;

	std::map<AssetRole, std::filesystem::path> assets;
	AddOptionalAsset(assets, AssetRole::Logo, imagesRoot / LogoName);
	AddOptionalAsset(assets, AssetRole::LogoInverse, imagesRoot / LogoInverseName);

	return assets;
}
} // namespace

Theme CreateInvestmentReportTheme(const std::filesystem::path& assetsRoot)
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