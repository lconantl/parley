#include "ai/DeepSeekClient.hpp"
#include "analyzer/ConversationGrouper.hpp"
#include "analyzer/DeepSeekSemanticSplitter.hpp"
#include "analyzer/DiagnosticPrinter.hpp"
#include "analyzer/IConversationGrouper.hpp"
#include "analyzer/IRawParser.hpp"
#include "analyzer/ISemanticClusterSplitter.hpp"
#include "analyzer/TelegramExportParser.hpp"
#include "console/ConsoleEncoding.hpp"
#include "http/HttplibHttpClient.hpp"

#include <iostream>
#include <memory>
#include <vector>

int main()
{
	ConsoleEncoding encoding;

	try
	{
		const std::unique_ptr<IRawParser> parser = std::make_unique<TelegramExportParser>();
		const std::vector<RawMessage> messages = parser->Parse("../res/result.json");

		const std::unique_ptr<IConversationGrouper> grouper = std::make_unique<ConversationGrouper>();
		std::vector<MessageCluster> clusters = grouper->Group(messages);

		DiagnosticPrinter::PrintClusterDiagnostics(clusters, messages.size());

		const Config config = Config::Load();
		std::unique_ptr<IHttpClient> httpClient = std::make_unique<HttplibHttpClient>();
		std::unique_ptr<IAIClient> aiClient = std::make_unique<DeepSeekClient>(config, std::move(httpClient));
		const std::unique_ptr<ISemanticClusterSplitter> splitter = std::make_unique<DeepSeekSemanticSplitter>(std::move(aiClient));
		const std::vector<SemanticClusterSplit> splits = splitter->Split(clusters, messages);
		DiagnosticPrinter::PrintSemanticSplits(splits);

		if (false)
		{
			DiagnosticPrinter::PrintClustersTop(clusters, messages);
			DiagnosticPrinter::PrintClusterById(clusters, messages, 42);
		}

	}
	catch (const std::exception& exception)
	{
		std::cerr << "[Error] " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}