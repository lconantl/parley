#include "Inn.hpp"
#include <cctype>
#include <vector>

namespace
{
constexpr std::size_t LegalLength = 10;
constexpr std::size_t IndividualLength = 12;

int ToDigit(const char symbol)
{
	return symbol - '0';
}

bool IsDigitsOnly(const std::string& value)
{
	for (const char symbol : value)
	{
		if (std::isdigit(static_cast<unsigned char>(symbol)) == 0)
		{
			return false;
		}
	}

	return !value.empty();
}

int CalculateControlDigit(const std::string& value, const std::vector<int>& weights)
{
	int sum = 0;

	for (std::size_t index = 0; index < weights.size(); ++index)
	{
		sum += weights[index] * ToDigit(value[index]);
	}

	return sum % 11 % 10;
}

bool IsLegalInnValid(const std::string& value)
{
	const std::vector weights{2, 4, 10, 3, 5, 9, 4, 6, 8};

	return CalculateControlDigit(value, weights) == ToDigit(value[9]);
}

bool IsIndividualInnValid(const std::string& value)
{
	const std::vector firstWeights{7, 2, 4, 10, 3, 5, 9, 4, 6, 8};
	const std::vector secondWeights{3, 7, 2, 4, 10, 3, 5, 9, 4, 6, 8};

	return CalculateControlDigit(value, firstWeights) == ToDigit(value[10])
		&& CalculateControlDigit(value, secondWeights) == ToDigit(value[11]);
}

bool IsSeparator(const char symbol)
{
	return std::isdigit(static_cast<unsigned char>(symbol)) == 0;
}
} // namespace

std::string Inn::Normalize(const std::string& value)
{
	std::string digits;

	for (const char symbol : value)
	{
		if (std::isdigit(static_cast<unsigned char>(symbol)) != 0)
		{
			digits += symbol;
		}
	}

	return digits;
}

bool Inn::IsValid(const std::string& value)
{
	if (!IsDigitsOnly(value))
	{
		return false;
	}

	if (value.size() == LegalLength)
	{
		return IsLegalInnValid(value);
	}

	if (value.size() == IndividualLength)
	{
		return IsIndividualInnValid(value);
	}

	return false;
}

std::string Inn::Extract(const std::string& text)
{
	std::size_t index = 0;

	while (index < text.size())
	{
		if (IsSeparator(text[index]))
		{
			++index;
			continue;
		}

		const std::size_t start = index;
		while (index < text.size() && !IsSeparator(text[index]))
		{
			++index;
		}

		const std::string candidate = text.substr(start, index - start);
		if (IsValid(candidate))
		{
			return candidate;
		}
	}

	return {};
}