#pragma once

#include <optional>
#include <string>
#include <vector>

struct SearchCriteria
{
	std::optional<std::string> name;
	std::optional<std::string> role;
	std::optional<std::string> location;
	std::vector<std::string> keywords;
};
