#pragma once

#include <string>

class Inn
{
public:
	static bool IsValid(const std::string& value);
	static std::string Extract(const std::string& text);
	static std::string Normalize(const std::string& value);
};