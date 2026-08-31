#include "Color.hpp"
#include <cctype>
#include <stdexcept>

namespace
{
constexpr std::size_t HEX_LENGTH = 6;
constexpr double CHANNEL_MAX = 255.0;

void AssertIsHexLength(const std::string_view hex)
{
	if (hex.size() != HEX_LENGTH)
	{
		throw std::invalid_argument("Цвет должен состоять ровно из шести шестнадцатеричных символов");
	}
}

void AssertIsHexDigit(const char symbol)
{
	if (std::isxdigit(static_cast<unsigned char>(symbol)) == 0)
	{
		throw std::invalid_argument("Цвет содержит недопустимый символ");
	}
}

unsigned ParseHexDigit(const char symbol)
{
	AssertIsHexDigit(symbol);

	if (symbol >= '0' && symbol <= '9')
	{
		return static_cast<unsigned>(symbol - '0');
	}

	const char lowered = static_cast<char>(std::tolower(static_cast<unsigned char>(symbol)));

	return static_cast<unsigned>(lowered - 'a') + 10u;
}

double ParseChannel(const std::string_view hex, const std::size_t offset)
{
	const unsigned high = ParseHexDigit(hex[offset]);
	const unsigned low = ParseHexDigit(hex[offset + 1]);

	return static_cast<double>(high * 16u + low) / CHANNEL_MAX;
}
} // namespace

Color ColorFromHex(const std::string_view hex)
{
	AssertIsHexLength(hex);

	return Color{ParseChannel(hex, 0), ParseChannel(hex, 2), ParseChannel(hex, 4)};
}