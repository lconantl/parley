#include "ConsoleCompanyStatisticsView.hpp"

#include <iostream>

void ConsoleCompanyStatisticsView::Show(
	const Company& company,
	const CheckoApiStatistics& statistics)
{
	ShowCompanyStatistics(company);
	ShowMethodStatistics(company);
	ShowApiStatistics(statistics);
}

void ConsoleCompanyStatisticsView::ShowCompanyStatistics(
	const Company& company)
{
	std::cout << "Организация: "
			  << company.GetIdentifier()
			  << std::endl;

	std::cout << "Получено методов: "
			  << company.GetMethodCount()
			  << std::endl;
}

void ConsoleCompanyStatisticsView::ShowMethodStatistics(
	const Company& company)
{
	const std::vector<std::string> methods = {
		"company",
		"timeline",
		"finances",
		"contracts",
		"inspections",
		"enforcements",
		"legal-cases",
		"fedresurs",
		"bankruptcy-messages"};

	for (const auto& method : methods)
	{
		std::cout << method
				  << ": "
				  << (company.HasData(method) ? "получен" : "отсутствует")
				  << std::endl;
	}
}

void ConsoleCompanyStatisticsView::ShowApiStatistics(
	const CheckoApiStatistics& statistics)
{
	std::cout << "Всего запросов: "
			  << statistics.requestCount
			  << std::endl;

	std::cout << "Успешных запросов: "
			  << statistics.successfulRequestCount
			  << std::endl;

	std::cout << "Ошибочных запросов: "
			  << statistics.failedRequestCount
			  << std::endl;
}