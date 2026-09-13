#pragma once

#include "application/importer/IContactSource.hpp"

class VCardContactsSource : public IContactSource
{
public:
	std::vector<RawContact> Load(const std::filesystem::path& path) const override;
};
