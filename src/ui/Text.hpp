#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace Text
{
std::string Escape(std::string_view value);
std::size_t Length(std::string_view value);
std::string Shorten(const std::string& value, std::size_t limit);
std::vector<std::string> Split(const std::string& value, std::size_t limit = 3500);
std::string Date(const std::string& iso);
}
