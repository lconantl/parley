#include "api/CheckoApiClient.hpp"
#include "bot/ParleyBot.hpp"
#include "common/console/ConsoleEncoding.hpp"
#include "viewmodel/CompanyProfileViewModel.hpp"
#include <iostream>
#include <tgbot/Bot.h>

int main()
{
	ConsoleEncoding encoding;

	try
	{
		std::cout << "Parley bot" << std::endl;

		const auto config = Config::LoadFromEnv(EnvLoader::FILE_NAME);

		const auto checkoClient = std::make_shared<CheckoApiClient>(config.GetCheckoApiKey());
		const auto dadataClient = std::make_shared<DaDataApiClient>(config.GetDaDataApiKey(), config.GetDaDataSecretKey());
		const auto polzaClient = std::make_shared<PolzaClient>(config.GetPolzaBaseUrl(), config.GetPolzaApiKey(), config.GetPolzaModel());

		const auto analyticsViewModel = std::make_shared<CompanyAnalyticsViewModel>(dadataClient, checkoClient, polzaClient);

		const MetricFormatOptions formatOptions;

		const ParleyBot bot(
			config,
			analyticsViewModel,
			formatOptions,
			std::filesystem::temp_directory_path() / "parley");

		bot.Run();
	}
	catch (const std::exception& exception)
	{
		std::cerr << "[Error] " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}