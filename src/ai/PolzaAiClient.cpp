#include "PolzaAiClient.hpp"

#include "config/Config.hpp"

#include <nlohmann/json.hpp>

#include <iostream>
#include <stdexcept>

namespace
{

constexpr long CONNECT_TIMEOUT_SEC = 15;
constexpr long READ_TIMEOUT_SEC = 180;
constexpr long WRITE_TIMEOUT_SEC = 30;

std::string Trim(
	const std::string& text)
{
	const auto start = text.find_first_not_of(" \t\r\n");

	if (start == std::string::npos)
	{
		return "";
	}

	const auto end = text.find_last_not_of(" \t\r\n");

	return text.substr(start, end - start + 1);
}

std::string StripMarkdownCodeFence(
	const std::string& text)
{
	std::string trimmed = Trim(text);

	if (!trimmed.starts_with("```"))
	{
		return trimmed;
	}

	const auto firstNewline = trimmed.find('\n');

	trimmed = firstNewline == std::string::npos
		? ""
		: trimmed.substr(firstNewline + 1);

	if (trimmed.ends_with("```"))
	{
		trimmed = trimmed.substr(0, trimmed.size() - 3);
	}

	return Trim(trimmed);
}

} // namespace

PolzaAiClient::PolzaAiClient(
	const Config& config)
	: m_httpClient(
		  config.GetPolzaBaseUrl(),
		  CONNECT_TIMEOUT_SEC,
		  READ_TIMEOUT_SEC,
		  WRITE_TIMEOUT_SEC,
		  "ParleyBot/1.0")
	, m_apiKey(config.GetPolzaApiKey())
	, m_model(config.GetPolzaModel())
{
}

nlohmann::json PolzaAiClient::CompleteJson(
	const std::string& systemPrompt,
	const std::string& userPrompt) const
{
	const nlohmann::json requestBody = {
		{"model", m_model},
		{"temperature", 0.2},
		{"messages", nlohmann::json::array({
						 {{"role", "system"}, {"content", systemPrompt}},
						 {{"role", "user"}, {"content", userPrompt}},
					 })},
	};

	const HttpResponse response = m_httpClient.Post(
		"/chat/completions",
		requestBody.dump(),
		"application/json",
		{{"Authorization", "Bearer " + m_apiKey}});

	std::cout
		<< "[Polza] HTTP: "
		<< response.status
		<< ", body: "
		<< response.body.size()
		<< " bytes"
		<< std::endl;

	if (
		response.status < 200
		|| response.status >= 300)
	{
		constexpr std::size_t MAX_RAW_ERROR_BODY_LENGTH = 300;

		try
		{
			const nlohmann::json errorRoot = nlohmann::json::parse(response.body);
			const auto errorIterator = errorRoot.find("error");

			if (
				errorIterator != errorRoot.end()
				&& errorIterator->is_object())
			{
				throw std::runtime_error(
					"Polza API: "
					+ errorIterator->value("code", std::string("UNKNOWN"))
					+ " — "
					+ errorIterator->value("message", std::string("без описания")));
			}
		}
		catch (const nlohmann::json::parse_error&)
		{
			// тело не JSON — падаем на общий фолбэк ниже
		}

		const std::string truncatedBody = response.body.size() > MAX_RAW_ERROR_BODY_LENGTH
			? response.body.substr(0, MAX_RAW_ERROR_BODY_LENGTH) + "..."
			: response.body;

		throw std::runtime_error(
			"Polza.ai вернул HTTP "
			+ std::to_string(response.status)
			+ ": "
			+ truncatedBody);
	}

	nlohmann::json root;

	try
	{
		root = nlohmann::json::parse(response.body);
	}
	catch (const nlohmann::json::parse_error& exception)
	{
		throw std::runtime_error(
			std::string("Не удалось разобрать ответ Polza.ai: ")
			+ exception.what());
	}

	const auto choicesIterator = root.find("choices");

	if (
		choicesIterator == root.end()
		|| !choicesIterator->is_array()
		|| choicesIterator->empty())
	{
		throw std::runtime_error("Polza.ai не вернул choices");
	}

	const std::string content = choicesIterator->front()
									.value("message", nlohmann::json::object())
									.value("content", std::string());

	try
	{
		return nlohmann::json::parse(
			StripMarkdownCodeFence(content));
	}
	catch (const nlohmann::json::parse_error&)
	{
		return nlohmann::json{{"raw_text", content}};
	}
}

const std::string& PolzaAiClient::GetModel() const
{
	return m_model;
}
