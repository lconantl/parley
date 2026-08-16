#include "bot/ParleyBot.hpp"
#include "config/Config.hpp"
#include "console/ConsoleEncoding.hpp"
#include <iostream>

int main()
{
	ConsoleEncoding encoding;

	try
	{
		std::cout << "Parley bot" << std::endl;

		const Config config = Config::Load();
		const ParleyBot bot(config);

		bot.Run();
	}
	catch (const std::exception& exception)
	{
		std::cerr << "[Error] " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}