#include "TelegramClient.hpp"

#include "TelegramError.hpp"
#include "system/StopSignal.hpp"
#include <curl/curl.h>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>

namespace
{
void AssertIsAvailable(bool available)
{
	if (!available)
	{
		throw std::runtime_error("Не удалось создать HTTP-запрос");
	}
}

void AssertIsSuccessful(CURLcode result)
{
	if (result != CURLE_OK)
	{
		throw std::runtime_error("Нет ответа от Telegram: ошибка соединения, код " +
								 std::to_string(static_cast<int>(result)));
	}
}

void AssertIsResponseValid(const nlohmann::json& response)
{
	if (!response.is_object() || !response.contains("ok") || !response["ok"].is_boolean())
	{
		throw std::runtime_error("Telegram вернул ответ в неизвестном формате");
	}
}

std::size_t WriteResponse(char* data, std::size_t size, std::size_t count, void* context) noexcept
{
	try
	{
		auto& body = *static_cast<std::string*>(context);
		const auto bytes = size * count;
		if (bytes > 8 * 1024 * 1024 || body.size() > 8 * 1024 * 1024 - bytes)
		{
			return 0;
		}
		body.append(data, bytes);
		return bytes;
	}
	catch (...)
	{
		return 0;
	}
}

int CheckStop(void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t) noexcept
{
	return StopSignal::IsRequested() ? 1 : 0;
}

nlohmann::json Post(const std::string& token, const std::string& method,
					const nlohmann::json& parameters)
{
	std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> handle(curl_easy_init(), curl_easy_cleanup);
	AssertIsAvailable(handle != nullptr);
	std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(
		curl_slist_append(nullptr, "Content-Type: application/json"), curl_slist_free_all);
	AssertIsAvailable(headers != nullptr);
	const auto url = "https://api.telegram.org/bot" + token + "/" + method;
	const auto body = parameters.dump();
	std::string response;
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_URL, url.c_str()));
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_POST, 1L));
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_POSTFIELDS, body.c_str()));
	AssertIsSuccessful(
		curl_easy_setopt(handle.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size())));
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_HTTPHEADER, headers.get()));
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, WriteResponse));
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, &response));
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYPEER, 1L));
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYHOST, 2L));
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_FOLLOWLOCATION, 0L));
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_NOSIGNAL, 1L));
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_NOPROGRESS, 0L));
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_XFERINFOFUNCTION, CheckStop));
	AssertIsSuccessful(curl_easy_setopt(handle.get(), CURLOPT_CONNECTTIMEOUT, 10L));
	AssertIsSuccessful(
		curl_easy_setopt(handle.get(), CURLOPT_TIMEOUT, method == "getUpdates" ? 40L : 25L));
	AssertIsSuccessful(curl_easy_perform(handle.get()));
	auto result = nlohmann::json::parse(response, nullptr, false);
	AssertIsResponseValid(result);
	return result;
}

void AssertIsAccepted(const nlohmann::json& response, const std::string& method)
{
	if (!response.value("ok", false))
	{
		throw TelegramError(response.value("error_code", 0), method,
							response.value("description", ""));
	}
}
}

TelegramClient::TelegramClient(std::string token) : m_token(std::move(token))
{
}

nlohmann::json TelegramClient::Call(const std::string& method, const nlohmann::json& parameters)
{
	if ((method == "sendMessage" || method == "editMessageText") && parameters.contains("chat_id"))
	{
		auto& next = m_nextSend[parameters["chat_id"].get<std::int64_t>()];
		std::this_thread::sleep_until(next);
		next = std::chrono::steady_clock::now() + std::chrono::milliseconds(1050);
	}
	for (int attempt = 0; attempt < 3; ++attempt)
	{
		const auto response = Post(m_token, method, parameters);
		if (response.value("error_code", 0) == 429 && attempt < 2)
		{
			const auto delay =
				response.value("parameters", nlohmann::json::object()).value("retry_after", 1);
			if (delay >= 0 && delay <= 60)
			{
				StopSignal::Wait(std::chrono::seconds(delay + 1));
				continue;
			}
		}
		AssertIsAccepted(response, method);
		return response.at("result");
	}
	return nullptr;
}
