#include "ai/DeepSeekClient.hpp"
#include "config/Config.hpp"
#include "console/ConsoleEncoding.hpp"
#include "http/HttplibHttpClient.hpp"
#include <iostream>
#include <memory>

int main()
{
	ConsoleEncoding encoding;

	try
	{
		const Config config = Config::Load();
		auto httpClient = std::make_unique<HttplibHttpClient>();

		DeepSeekClient aiClient(config, std::move(httpClient));

		std::cout << "Отправка тестового запроса к DeepSeek..." << std::endl;

		const std::string response = aiClient.Complete(
			"You are a helpful assistant. Reply with JSON only.",
			"Return JSON: {\"ok\": true}",
			true
		);

		std::cout << "Ответ модели:\n" << response << std::endl;
	}
	catch (const std::exception& exception)
	{
		std::cerr << "[Error] " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}