#pragma once

#include <string>

enum class MetricOrigin
{
	Unavailable,
	Reported,
	Computed,
	Estimated,
	NotApplicable
};

enum class MetricGroup
{
	Revenue,
	Profit,
	Margins,
	CashFlow,
	WorkingCapital,
	Debt,
	Balance,
	Liquidity,
	Returns,
	Investment,
	Shareholders,
	Valuation,
	Quality,
	Market
};

enum class MetricUnit
{
	Money,
	Percent,
	Ratio,
	Days,
	Count,
	Text
};

struct MetricValue
{
	double value = 0.0;
	MetricOrigin origin = MetricOrigin::Unavailable;
	double confidence = 0.0;
	std::string comment;
};
