#pragma once

#include "domain/search/SearchResult.hpp"

#include <string>
#include <vector>

class ContactResultFormatter
{
public:
	static std::string Format(const std::vector<SearchResult>& results);

private:
	static std::string FormatOne(std::size_t index, const Contact& contact);
	static std::string FindChannelValue(const Contact& contact, ChannelType type);
};
