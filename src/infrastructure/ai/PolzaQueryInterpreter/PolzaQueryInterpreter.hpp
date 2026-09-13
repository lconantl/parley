#pragma once

#include "application/interpreter/IQueryInterpreter.hpp"

#include <memory>
#include <string>

class PolzaQueryInterpreter : public IQueryInterpreter
{
public:
	PolzaQueryInterpreter(std::string baseUrl, std::string apiKey, std::string model);
	~PolzaQueryInterpreter() override;

	SearchCriteria Interpret(const std::string& text) const override;

private:
	class Impl;

	std::unique_ptr<Impl> m_impl;
};
