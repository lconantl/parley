#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

struct FinancialYear
{
	int year = 0;
	std::unordered_map<int, double> lines;
};

class FinancialHistory
{
public:
	void AddYear(FinancialYear year);
	void MergeYear(const FinancialYear& year);

	bool IsEmpty() const noexcept;
	std::size_t GetYearCount() const noexcept;
	std::vector<int> GetYears() const;
	int GetLatestYear() const;

	bool HasYear(int year) const;
	const FinancialYear& GetYear(int year) const;
	bool HasLine(int year, int code) const;
	std::optional<double> FindLine(int year, int code) const;

private:
	std::map<int, FinancialYear, std::greater<int>> m_years;
};