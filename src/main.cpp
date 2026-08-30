#include "bot/ParleyBot.hpp"
#include "config/Config.hpp"
#include "config/EnvLoader.hpp"
#include "console/ConsoleEncoding.hpp"
#include <iostream>
#include <tgbot/Bot.h>

int main()
{
	ConsoleEncoding encoding;

	try
	{

		std::cout << "Parley bot" << std::endl;

		const Config config = Config::LoadFromEnv(EnvLoader::FILE_NAME);
		const ParleyBot bot(config.GetBotToken());

		bot.Run();
	}
	catch (const std::exception& exception)
	{
		std::cerr << "[Error] " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}