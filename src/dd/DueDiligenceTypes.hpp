#pragma once

#include <optional>
#include <string>

struct DueDiligenceMetric
{
	std::string name;
	std::optional<double> value;
	std::string availability;
	std::optional<std::string> reason;
};

struct DueDiligenceFlag
{
	std::string code;
	std::string severity;
	std::string dimension;
	std::string explanation;
};
