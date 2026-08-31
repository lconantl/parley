#pragma once

#include "CompanyAnalytics.hpp"
#include "FinancialHistory.hpp"
#include "model/CompanyProfile.hpp"

class FinancialCalculator
{
public:
	static CompanyAnalytics Extract(
		const FinancialHistory& history,
		const CompanyProfile& profile,
		int year);

	static void Derive(CompanyAnalytics& analytics);
};