#include "FinancialHistory.hpp"

#include <stdexcept>
#include <utility>

namespace
{
void AssertIsYearValid(const int year)
{
	if (year < 1990 || year > 2100)
	{
		throw std::invalid_argument("Год отчетности вне допустимого диапазона");
	}
}

void AssertIsHistoryNotEmpty(const bool empty)
{
	if (empty)
	{
		throw std::out_of_range("История финансовой отчетности пуста");
	}
}

void AssertIsYearPresent(const bool present)
{
	if (!present)
	{
		throw std::out_of_range("Отчетность за указанный год отсутствует");
	}
}
} // namespace

void FinancialHistory::AddYear(FinancialYear year)
{
	AssertIsYearValid(year.year);
	const int key = year.year;
	m_years[key] = std::move(year);
}

void FinancialHistory::MergeYear(const FinancialYear& year)
{
	AssertIsYearValid(year.year);

	FinancialYear& target = m_years[year.year];
	target.year = year.year;

	for (const auto& [code, value] : year.lines)
	{
		target.lines.emplace(code, value);
	}
}

bool FinancialHistory::IsEmpty() const noexcept
{
	return m_years.empty();
}

std::size_t FinancialHistory::GetYearCount() const noexcept
{
	return m_years.size();
}

std::vector<int> FinancialHistory::GetYears() const
{
	std::vector<int> years;
	years.reserve(m_years.size());

	for (const auto& [year, report] : m_years)
	{
		years.push_back(year);
	}

	return years;
}

int FinancialHistory::GetLatestYear() const
{
	AssertIsHistoryNotEmpty(m_years.empty());

	return m_years.begin()->first;
}

bool FinancialHistory::HasYear(const int year) const
{
	return m_years.contains(year);
}

const FinancialYear& FinancialHistory::GetYear(const int year) const
{
	const auto iterator = m_years.find(year);
	AssertIsYearPresent(iterator != m_years.end());

	return iterator->second;
}

bool FinancialHistory::HasLine(const int year, const int code) const
{
	return FindLine(year, code).has_value();
}

std::optional<double> FinancialHistory::FindLine(const int year, const int code) const
{
	const auto reportIterator = m_years.find(year);
	if (reportIterator == m_years.end())
	{
		return std::nullopt;
	}

	const auto lineIterator = reportIterator->second.lines.find(code);
	if (lineIterator == reportIterator->second.lines.end())
	{
		return std::nullopt;
	}

	return lineIterator->second;
}