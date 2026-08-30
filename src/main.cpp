#include "api/CheckoApiClient.hpp"
#include "bot/ParleyBot.hpp"
#include "config/Config.hpp"
#include "config/EnvLoader.hpp"
#include "console/ConsoleEncoding.hpp"
#include "pdf/render/PdfGenerator.hpp"
#include "pdf/theme/StrategyPartnersTheme.hpp"
#include "view/ConsoleCompanyStatisticsView.hpp"
#include "viewmodel/CompanyViewModel.hpp"

#include <iostream>
#include <tgbot/Bot.h>

int main()
{
	ConsoleEncoding encoding;

	try
	{
		std::cout << "Parley bot" << std::endl;

		const auto config = Config::LoadFromEnv(EnvLoader::FILE_NAME);
		const auto apiClient = std::make_shared<CheckoApiClient>(config.GetCheckoApiKey());

		// // 1. Инициализация темы
		// Theme theme = CreateStrategyPartnersTheme("./assets");
		//
		// // 2. Создание макета слайда
		// CardGridSlideContent content;
		// content.title = "Направления работы";
		// content.columnCount = 3;
		// content.cards = {{"Стратегия", "Описание блока"}};
		//
		// // 3. Формирование презентации
		// Deck deck;
		// deck.title = "Обучающая презентация";
		// deck.slides.push_back(Slide{content});
		//
		// // 4. Сохранение PDF
		// PdfGenerator::Generate(deck, theme, "output.pdf");

		const CompanyViewModel viewModel(apiClient);
		const auto company = viewModel.LoadCompany("1215139170");
		ConsoleCompanyStatisticsView view;
		view.Show(*company, apiClient->GetStatistics());

		// company->SaveToJson("data.json");
		// std::cout << "Данные успешно сохранены в data.json" << std::endl;
		// const ParleyBot bot(config.GetBotToken());
		// bot.Run();
	}
	catch (const std::exception& exception)
	{
		std::cerr << "[Error] " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}