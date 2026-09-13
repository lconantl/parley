#pragma once

#include "application/importer/IContactSource.hpp"

class ExcelContactsSource : public IContactSource
{
public:
	std::vector<RawContact> Load(const std::filesystem::path& path) const override;
};
