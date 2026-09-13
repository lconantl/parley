#pragma once

#include "domain/search/SearchCriteria.hpp"

#include <string>

class IQueryInterpreter
{
public:
	virtual ~IQueryInterpreter() = default;

	virtual SearchCriteria Interpret(const std::string& text) const = 0;
};
