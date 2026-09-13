#pragma once

#include "application/repository/IContactRepository.hpp"
#include "infrastructure/storage/SqliteConnection/SqliteConnection.hpp"

#include <filesystem>
#include <memory>

class SqliteContactRepository : public IContactRepository
{
public:
	explicit SqliteContactRepository(const std::filesystem::path& databasePath);

	std::optional<Contact> FindByChannel(const ContactChannel& channel) const override;
	Contact Upsert(const Contact& contact) override;
	std::vector<SearchResult> Search(const SearchCriteria& criteria, std::size_t limit) const override;

private:
	void CreateSchema() const;
	Contact LoadContact(std::int64_t contactId) const;
	std::vector<ContactChannel> LoadChannels(std::int64_t contactId) const;
	std::vector<ContactSource> LoadSources(std::int64_t contactId) const;
	std::int64_t SaveContactRow(const Contact& contact) const;
	void ReplaceChannels(std::int64_t contactId, const std::vector<ContactChannel>& channels) const;
	void ReplaceSources(std::int64_t contactId, const std::vector<ContactSource>& sources) const;
	void SyncFts(const Contact& contact) const;

	std::unique_ptr<SqliteConnection> m_connection;
};
