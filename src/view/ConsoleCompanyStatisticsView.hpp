#pragma once
#include "api/CheckoApiTypes.hpp"
#include "model/Company.hpp"

class ConsoleCompanyStatisticsView
{
public:
	static void Show(
		const Company& company,
		const CheckoApiStatistics& statistics);

private:
	static void ShowCompanyStatistics(const Company& company);
	static void ShowMethodStatistics(const Company& company);
	static void ShowApiStatistics(const CheckoApiStatistics& statistics);
};