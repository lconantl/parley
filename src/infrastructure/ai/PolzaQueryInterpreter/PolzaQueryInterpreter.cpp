#include "PolzaQueryInterpreter.hpp"
#include "infrastructure/ai/polza/polza.hpp"

#include <stdexcept>
#include <string_view>
#include <utility>

namespace
{
constexpr long RequestTimeoutSeconds = 60;
constexpr auto SchemaName = "search_criteria";
constexpr std::string_view VersionSuffix = "/v1";

constexpr auto SystemPrompt =
	"Ты извлекаешь параметры поиска по базе контактов из запроса пользователя на русском языке. "
	"Верни JSON: name — имя человека, если явно указано, иначе null; role — профессия/роль, иначе null; "
	"location — город или регион, иначе null; keywords — список остальных значимых слов запроса "
	"(навыки, интересы, теги) без повторения name/role/location. Не выдумывай данные, которых нет в запросе.";

void AssertIsNotEmpty(const std::string& value, const std::string& message)
{
	if (value.empty())
	{
		throw std::invalid_argument(message);
	}
}

std::string NormalizeBaseUrl(std::string baseUrl)
{
	while (!baseUrl.empty() && baseUrl.back() == '/')
	{
		baseUrl.pop_back();
	}

	if (baseUrl.size() >= VersionSuffix.size()
		&& baseUrl.compare(baseUrl.size() - VersionSuffix.size(), VersionSuffix.size(), VersionSuffix) == 0)
	{
		baseUrl.erase(baseUrl.size() - VersionSuffix.size());
	}

	return baseUrl;
}

nlohmann::json BuildSchema()
{
	return {
		{"type", "object"},
		{"properties",
			{{"name", {{"type", {"string", "null"}}}},
				{"role", {{"type", {"string", "null"}}}},
				{"location", {{"type", {"string", "null"}}}},
				{"keywords", {{"type", "array"}, {"items", {{"type", "string"}}}}}}},
		{"required", {"name", "role", "location", "keywords"}},
		{"additionalProperties", false}};
}

SearchCriteria ParseCriteria(const nlohmann::json& body)
{
	SearchCriteria criteria;

	if (body.contains("name") && body.at("name").is_string())
	{
		criteria.name = body.at("name").get<std::string>();
	}
	if (body.contains("role") && body.at("role").is_string())
	{
		criteria.role = body.at("role").get<std::string>();
	}
	if (body.contains("location") && body.at("location").is_string())
	{
		criteria.location = body.at("location").get<std::string>();
	}
	if (body.contains("keywords") && body.at("keywords").is_array())
	{
		for (const auto& keyword : body.at("keywords"))
		{
			if (keyword.is_string())
			{
				criteria.keywords.push_back(keyword.get<std::string>());
			}
		}
	}

	return criteria;
}
} // namespace

class PolzaQueryInterpreter::Impl
{
public:
	Impl(std::string baseUrl, std::string apiKey, std::string model)
		: m_model(std::move(model))
		, m_client(MakeOptions(std::move(baseUrl), std::move(apiKey)))
	{
		AssertIsNotEmpty(m_model, "Модель нейросети не может быть пустой");
	}

	SearchCriteria Interpret(const std::string& text) const
	{
		nlohmann::json body;
		body["model"] = m_model;
		body["messages"] = nlohmann::json::array({
			{{"role", "system"}, {"content", SystemPrompt}},
			{{"role", "user"}, {"content", text}},
		});
		body["response_format"] = {
			{"type", "json_schema"},
			{"json_schema", {{"name", SchemaName}, {"strict", true}, {"schema", BuildSchema()}}}};

		const nlohmann::json completion = m_client.chat(body);
		const std::string content = polza::Client::content_of(completion);

		return ParseCriteria(nlohmann::json::parse(content));
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

	std::string m_model;
	polza::Client m_client;
};

PolzaQueryInterpreter::PolzaQueryInterpreter(std::string baseUrl, std::string apiKey, std::string model)
	: m_impl(std::make_unique<Impl>(std::move(baseUrl), std::move(apiKey), std::move(model)))
{
}

PolzaQueryInterpreter::~PolzaQueryInterpreter() = default;

SearchCriteria PolzaQueryInterpreter::Interpret(const std::string& text) const
{
	AssertIsNotEmpty(text, "Текст запроса не может быть пустым");

	return m_impl->Interpret(text);
}
