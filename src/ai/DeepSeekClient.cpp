#include "DeepSeekClient.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace
{
void AssertIsStatusOk(const int status, const std::string& body)
{
	if (status < 200 || status >= 300)
	{
		std::string errorMessage;
		if (status == 401) errorMessage = "Ошибка авторизации (401). Неверный API ключ.";
		else if (status == 429) errorMessage = "Превышен лимит запросов (429 Too Many Requests).";
		else if (status >= 400 && status < 500) errorMessage = "Ошибка запроса клиента (" + std::to_string(status) + ").";
		else if (status >= 500) errorMessage = "Внутренняя ошибка сервера (" + std::to_string(status) + ").";
		else errorMessage = "Неизвестная ошибка HTTP: " + std::to_string(status);

		throw std::runtime_error(errorMessage + " Тело ответа: " + body);
	}
}

nlohmann::json ParseResponseBody(const std::string& body)
{
	try
	{
		return nlohmann::json::parse(body);
	}
	catch (const nlohmann::json::parse_error&)
	{
		throw std::runtime_error("Сервер вернул невалидный JSON: " + body);
	}
}

void AssertHasContent(const nlohmann::json& responseRoot)
{
	if (!responseRoot.contains("choices") || !responseRoot["choices"].is_array() || responseRoot["choices"].empty())
	{
		throw std::runtime_error("В ответе API отсутствует массив 'choices'");
	}

	const auto& choice = responseRoot["choices"][0];
	if (!choice.contains("message") || !choice["message"].contains("content") || !choice["message"]["content"].is_string())
	{
		throw std::runtime_error("В ответе API отсутствует текстовое содержимое ('message.content')");
	}
}

std::string ExtractContent(const std::string& body)
{
	const nlohmann::json root = ParseResponseBody(body);
	AssertHasContent(root);
	return root["choices"][0]["message"]["content"].get<std::string>();
}

std::string BuildEndpointUrl(const std::string& baseUrl)
{
	if (baseUrl.ends_with('/'))
	{
		return baseUrl + "chat/completions";
	}
	return baseUrl + "/chat/completions";
}

std::string BuildRequestBody(
	const std::string& model,
	const std::string& systemPrompt,
	const std::string& userPrompt,
	const bool jsonMode)
{
	nlohmann::json body;
	body["model"] = model;
	body["temperature"] = 0;

	if (jsonMode)
	{
		body["response_format"]["type"] = "json_object";
	}

	nlohmann::json systemMessage;
	systemMessage["role"] = "system";
	systemMessage["content"] = systemPrompt;

	nlohmann::json userMessage;
	userMessage["role"] = "user";
	userMessage["content"] = userPrompt;

	body["messages"] = nlohmann::json::array({systemMessage, userMessage});

	return body.dump();
}

std::vector<HttpHeader> BuildHeaders(const std::string& apiKey)
{
	return {
		{"Authorization", "Bearer " + apiKey},
		{"Accept", "application/json"}
	};
}
} // namespace

DeepSeekClient::DeepSeekClient(const Config& config, std::unique_ptr<IHttpClient> httpClient)
	: m_config(config)
	, m_httpClient(std::move(httpClient))
{
	if (!m_httpClient)
	{
		throw std::runtime_error("Указатель на HTTP клиент не может быть пустым");
	}
}

DeepSeekClient::~DeepSeekClient()
{
}

std::string DeepSeekClient::Complete(const std::string& systemPrompt, const std::string& userPrompt, const bool jsonMode)
{
	const std::string url = BuildEndpointUrl(m_config.GetDeepSeekBaseUrl());
	const std::string body = BuildRequestBody(m_config.GetDeepSeekModel(), systemPrompt, userPrompt, jsonMode);
	const std::vector<HttpHeader> headers = BuildHeaders(m_config.GetDeepSeekApiKey());

	const HttpResponse response = m_httpClient->Post(url, headers, body);
	AssertIsStatusOk(response.statusCode, response.body);

	return ExtractContent(response.body);
}