#include "config/Config.hpp"
#include "config/EnvLoader.hpp"
#include "console/ConsoleEncoding.hpp"
#include <iostream>

int main()
{
	ConsoleEncoding encoding;

	try
	{
		std::cout << "Parley bot" << std::endl;
		const Config config = Config::LoadFromEnv(EnvLoader::FILE_NAME);
		std::cout << config.GetBotToken() << std::endl;
		std::cout << config.GetAllowedUsers()[2] << std::endl;
		std::cout << config.GetPolzaBaseUrl() << std::endl;
		std::cout << config.GetPolzaApiKey() << std::endl;
		std::cout << config.GetPolzaModel() << std::endl;
	}
	catch (const std::exception& exception)
	{
		std::cerr << "[Error] " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}