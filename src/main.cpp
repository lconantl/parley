#include "analyzer/TelegramAnalyzer.hpp"
#include "console/ConsoleEncoding.hpp"
#include <iostream>

int main()
{
	ConsoleEncoding encoding;

	try
	{
		TelegramAnalyzer::PrintReport("res");
	}
	catch (const std::exception& exception)
	{
		std::cerr << "[Error] " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}