#pragma once

#include "application/importer/IContactSource.hpp"
#include "application/repository/IContactRepository.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>

class ContactImportService
{
public:
	explicit ContactImportService(std::shared_ptr<IContactRepository> repository);

	std::size_t ImportFrom(const IContactSource& source, const std::filesystem::path& path) const;

private:
	void ImportOne(const RawContact& raw) const;
	Contact MergeInto(Contact existing, const Contact& incoming) const;

	std::shared_ptr<IContactRepository> m_repository;
};
