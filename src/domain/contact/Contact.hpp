#pragma once

#include "ContactChannel.hpp"
#include "ContactSource.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct Contact
{
	std::int64_t id = 0;
	std::string canonicalName;
	std::string role;
	std::string location;
	std::string notes;
	std::vector<ContactChannel> channels;
	std::vector<ContactSource> sources;
};
