#pragma once

#include "RawContact.hpp"

#include <filesystem>
#include <vector>

class IContactSource
{
public:
	virtual ~IContactSource() = default;

	virtual std::vector<RawContact> Load(const std::filesystem::path& path) const = 0;
};
