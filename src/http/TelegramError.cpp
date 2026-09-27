#include "TelegramError.hpp"

TelegramError::TelegramError(int code, const std::string& method, const std::string& description)
	: std::runtime_error("Telegram отклонил запрос " + method + ", код " + std::to_string(code)),
	  m_code(code), m_unchanged(description.find("message is not modified") != std::string::npos),
	  m_missingMessage(description.find("message to edit not found") != std::string::npos ||
					   description.find("message can't be edited") != std::string::npos)
{
}

int TelegramError::GetCode() const noexcept
{
	return m_code;
}

bool TelegramError::IsUnchanged() const noexcept
{
	return m_unchanged;
}

bool TelegramError::IsMissingMessage() const noexcept
{
	return m_missingMessage;
}
