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

void AssertIsNotEmpty(const std::string& value, const std::string& message)
{
	if (value.empty())
	{
		throw std::invalid_argument(message);
	}
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
		options.base_url = std::move(baseUrl);
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