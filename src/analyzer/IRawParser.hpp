#pragma once

#include "RawMessage.hpp"
#include <filesystem>
#include <vector>

class IRawParser
{
public:
	virtual ~IRawParser() = default;

	virtual std::vector<RawMessage> Parse(const std::filesystem::path& path) const = 0;
};