#pragma once

#include <stdexcept>
#include <string>

class TelegramError : public std::runtime_error
{
  public:
	TelegramError(int code, const std::string& method, const std::string& description);
	int GetCode() const noexcept;
	bool IsUnchanged() const noexcept;
	bool IsMissingMessage() const noexcept;

  private:
	int m_code;
	bool m_unchanged;
	bool m_missingMessage;
};
