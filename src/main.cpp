#include "console/ConsoleEncoding.hpp"
#include <iostream>

int main()
{
	ConsoleEncoding encoding;

	try
	{
		std::cout << "parley" << std::endl;
	}
	catch (std::exception& e)
	{
		std::cerr << "[Error] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}