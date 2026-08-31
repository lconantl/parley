#include "ai/DueDiligenceNarrator.hpp"
#include "ai/PolzaClient.hpp"
#include "api/CheckoApiClient.hpp"
#include "api/DaDataApiClient.hpp"
#include "bot/ParleyBot.hpp"
#include "common/config/Config.hpp"
#include "common/config/EnvLoader.hpp"
#include "common/console/ConsoleEncoding.hpp"
#include "common/output/DueDiligenceDeckBuilder.hpp"
#include "common/output/pdf/theme/InvestmentReportTheme.hpp"
#include "finance/MetricFormatter.hpp"
#include "viewmodel/CompanyAnalyticsViewModel.hpp"

#include <filesystem>
#include <iostream>
#include <memory>

namespace
{
constexpr auto AssetsFolder = "assets";
constexpr auto OutputFolder = "parley";
constexpr int MaxSearchDepth = 4;

std::filesystem::path ResolveAssetsRoot()
{
	std::filesystem::path candidate = std::filesystem::current_path();

	for (int depth = 0; depth <= MaxSearchDepth; ++depth)
	{
		const std::filesystem::path assets = candidate / AssetsFolder;
		if (std::filesystem::is_directory(assets))
		{
			return assets;
		}

		if (!candidate.has_parent_path() || candidate.parent_path() == candidate)
		{
			break;
		}

		candidate = candidate.parent_path();
	}

	throw std::runtime_error(
		"Каталог assets не найден рядом с рабочим каталогом: "
		+ std::filesystem::current_path().string());
}

MetricFormatOptions CreateFormatOptions()
{
	MetricFormatOptions options;
	options.showOrigin = false;
	options.showConfidence = false;
	options.showMissing = false;
	options.showNotApplicable = false;
	options.compactMoney = true;

	return options;
}

DueDiligenceOptions CreateReportOptions()
{
	DueDiligenceOptions options;
	options.anonymize = true;
	options.showSourceNotes = true;

	return options;
}
} // namespace

int main()
{
	ConsoleEncoding encoding;

	try
	{
		std::cout << "Parley bot" << std::endl;

		const auto config = Config::LoadFromEnv(EnvLoader::FILE_NAME);

		const auto checkoClient = std::make_shared<CheckoApiClient>(config.GetCheckoApiKey());
		const auto dadataClient = std::make_shared<DaDataApiClient>(
			config.GetDaDataApiKey(),
			config.GetDaDataSecretKey());
		const auto polzaClient = std::make_shared<PolzaClient>(
			config.GetPolzaBaseUrl(),
			config.GetPolzaApiKey(),
			config.GetPolzaModel());

		polzaClient->AssertIsModelAvailable();

		const auto analyticsViewModel = std::make_shared<CompanyAnalyticsViewModel>(
			dadataClient,
			checkoClient,
			polzaClient);

		const auto narrator = std::make_shared<DueDiligenceNarrator>(polzaClient);

		ParleyBot::Dependencies dependencies;
		dependencies.analyticsViewModel = analyticsViewModel;
		dependencies.narrator = narrator;
		dependencies.theme = CreateInvestmentReportTheme(ResolveAssetsRoot());
		dependencies.formatOptions = CreateFormatOptions();
		dependencies.reportOptions = CreateReportOptions();
		dependencies.outputDirectory = std::filesystem::temp_directory_path() / OutputFolder;

		const ParleyBot bot(config, dependencies);
		bot.Run();
	}
	catch (const std::exception& exception)
	{
		std::cerr << "[Error] " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}