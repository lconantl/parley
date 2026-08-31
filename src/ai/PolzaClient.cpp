#include "PolzaClient.hpp"
#include "common/polza/polza.hpp"
#include <atomic>
#include <stdexcept>
#include <utility>

namespace
{
constexpr long RequestTimeoutSeconds = 240;
constexpr auto WebSearchPluginId = "web";
constexpr auto HealingPluginId = "response-healing";
constexpr auto ModelsEndpoint = "/v1/models?type=chat";
constexpr auto VersionSuffix = "/v1";
constexpr std::size_t MaxSuggestedModels = 10;

void AssertIsNotEmpty(const std::string& value, const std::string& message)
{
	if (value.empty())
	{
		throw std::invalid_argument(message);
	}
}

bool EndsWith(const std::string& text, const std::string& suffix)
{
	if (text.size() < suffix.size())
	{
		return false;
	}

	return text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

nlohmann::json BuildMessages(const std::string& systemPrompt, const std::string& userPrompt)
{
	return nlohmann::json::array({
		{{"role", "system"}, {"content", systemPrompt}},
		{{"role", "user"}, {"content", userPrompt}},
	});
}

std::string ExtractContent(const nlohmann::json& completion)
{
	return polza::Client::content_of(completion);
}

double ExtractCost(const nlohmann::json& completion)
{
	return polza::parse_usage(completion).cost_rub;
}
} // namespace

class PolzaClient::Impl
{
public:
	Impl(std::string baseUrl, std::string apiKey, std::string model)
		: m_model(std::move(model))
		, m_client(MakeOptions(std::move(baseUrl), std::move(apiKey)))
	{
		AssertIsNotEmpty(m_model, "Модель нейросети не может быть пустой");
	}

	PolzaAnswer Ask(const nlohmann::json& body)
	{
		const nlohmann::json completion = m_client.chat(body);
		const double cost = ExtractCost(completion);

		AddCost(cost);

		return {ExtractContent(completion), cost};
	}

	const std::string& GetModel() const noexcept
	{
		return m_model;
	}

	nlohmann::json Get(const std::string& path) const
	{
		return m_client.get(path);
	}

	double GetTotalCost() const
	{
		return m_totalCost.load();
	}

private:
	static polza::Options MakeOptions(std::string baseUrl, std::string apiKey)
	{
		AssertIsNotEmpty(apiKey, "Ключ доступа к нейросети не может быть пустым");
		AssertIsNotEmpty(baseUrl, "Адрес нейросети не может быть пустым");

		polza::Options options;
		options.api_key = std::move(apiKey);
		options.base_url = NormalizeBaseUrl(std::move(baseUrl));
		options.timeout_seconds = RequestTimeoutSeconds;

		return options;
	}

	void AddCost(const double cost)
	{
		double expected = m_totalCost.load();
		while (!m_totalCost.compare_exchange_weak(expected, expected + cost))
		{
		}
	}

private:
	std::string m_model;
	polza::Client m_client;
	std::atomic<double> m_totalCost{0.0};
};

PolzaClient::PolzaClient(std::string baseUrl, std::string apiKey, std::string model)
	: m_impl(std::make_unique<Impl>(std::move(baseUrl), std::move(apiKey), std::move(model)))
{
}

PolzaClient::~PolzaClient() = default;

PolzaAnswer PolzaClient::AskWithSearch(
	const std::string& systemPrompt,
	const std::string& userPrompt,
	const std::size_t maxSearchResults) const
{
	nlohmann::json body;
	body["model"] = m_impl->GetModel();
	body["messages"] = BuildMessages(systemPrompt, userPrompt);
	body["plugins"] = nlohmann::json::array({
		{{"id", WebSearchPluginId}, {"max_results", maxSearchResults}},
	});

	return m_impl->Ask(body);
}

PolzaAnswer PolzaClient::AskStructured(
	const std::string& systemPrompt,
	const std::string& userPrompt,
	const std::string& schemaName,
	const nlohmann::json& schema) const
{
	AssertIsNotEmpty(schemaName, "Имя схемы ответа не может быть пустым");

	nlohmann::json body;
	body["model"] = m_impl->GetModel();
	body["messages"] = BuildMessages(systemPrompt, userPrompt);
	body["plugins"] = nlohmann::json::array({
		{{"id", HealingPluginId}},
	});
	body["response_format"] = {
		{"type", "json_schema"},
		{"json_schema",
			{{"name", schemaName},
				{"strict", true},
				{"schema", schema}}}};

	return m_impl->Ask(body);
}

double PolzaClient::GetTotalCost() const
{
	return m_impl->GetTotalCost();
}

std::string PolzaClient::NormalizeBaseUrl(std::string baseUrl)
{
	while (!baseUrl.empty() && baseUrl.back() == '/')
	{
		baseUrl.pop_back();
	}

	if (EndsWith(baseUrl, VersionSuffix))
	{
		baseUrl.erase(baseUrl.size() - std::string(VersionSuffix).size());
	}

	AssertIsNotEmpty(baseUrl, "Адрес нейросети не может быть пустым");

	return baseUrl;
}

std::vector<std::string> PolzaClient::ListChatModels() const
{
	const nlohmann::json catalogue = m_impl->Get(ModelsEndpoint);

	std::vector<std::string> models;
	if (!catalogue.contains("data") || !catalogue.at("data").is_array())
	{
		return models;
	}

	for (const auto& node : catalogue.at("data"))
	{
		if (node.contains("id") && node.at("id").is_string())
		{
			models.push_back(node.at("id").get<std::string>());
		}
	}

	return models;
}

void PolzaClient::AssertIsModelAvailable() const
{
	const std::vector<std::string> models = ListChatModels();
	if (models.empty())
	{
		return;
	}

	const std::string& selected = m_impl->GetModel();
	for (const auto& model : models)
	{
		if (model == selected)
		{
			return;
		}
	}

	std::string message = "Модель " + selected + " недоступна. Доступны, например:";
	std::size_t counter = 0;

	for (const auto& model : models)
	{
		if (counter >= MaxSuggestedModels)
		{
			break;
		}

		message += " " + model;
		++counter;
	}

	throw std::runtime_error(message);
}