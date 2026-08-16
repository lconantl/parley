#include "analyzer/IRawParser.hpp"
#include "analyzer/RawMessage.hpp"
#include "analyzer/TelegramAnalyzer.hpp"
#include "analyzer/TelegramExportParser.hpp"
#include "console/ConsoleEncoding.hpp"
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

namespace
{
void PrintMessagesSummary(const std::vector<RawMessage>& messages, const size_t limit)
{
	std::cout << "\nTotal parsed messages: " << messages.size() << std::endl;
	const size_t displayCount = std::min(messages.size(), limit);
	std::cout << "Будет показано: " << displayCount << std::endl;

	std::cout << "---------------------------" << std::endl;

	for (size_t i = 0; i < displayCount; ++i)
	{
		const auto& message = messages[i];

		std::cout << "Message ID: " << message.id.value_or(0) << std::endl;
		std::cout << "Type: " << message.type.value_or("N/A") << std::endl;
		std::cout << "Date: " << message.date.value_or("N/A") << std::endl;
		std::cout << "From: " << message.from.value_or("N/A") << std::endl;
		std::cout << "Text entities count: " << message.textEntities.size() << std::endl;
		std::cout << "Extra fields count: " << message.extraFields.size() << std::endl;
		std::cout << "Raw JSON size (bytes): " << message.rawJson.dump().size() << std::endl;
		std::cout << "---------------------------" << std::endl;
	}
}
} // namespace

int main()
{
	ConsoleEncoding encoding;

	try
	{
		TelegramAnalyzer::PrintReport("res");
		const auto parser = std::make_unique<TelegramExportParser>();
		const auto messages = parser->Parse("res/result.json");
		PrintMessagesSummary(messages, 1);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "[Error] " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}