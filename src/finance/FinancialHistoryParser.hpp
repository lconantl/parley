#pragma once

#include "FinancialHistory.hpp"

#include <nlohmann/json.hpp>

class FinancialHistoryParser
{
public:
	static FinancialHistory ParseChecko(const nlohmann::json& response);
	static FinancialHistory ParseDaData(const nlohmann::json& response);
	static FinancialHistory Merge(const FinancialHistory& primary, const FinancialHistory& secondary);
};