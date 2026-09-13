#include "SqliteContactRepository.hpp"
#include "infrastructure/storage/SqliteStatement/SqliteStatement.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace
{
constexpr std::size_t MinStemCodepoints = 4;

std::size_t Utf8CodepointLength(const unsigned char leadByte)
{
	if ((leadByte & 0xE0) == 0xC0)
	{
		return 2;
	}
	if ((leadByte & 0xF0) == 0xE0)
	{
		return 3;
	}
	if ((leadByte & 0xF8) == 0xF0)
	{
		return 4;
	}

	return 1;
}

std::size_t Utf8CodepointCount(const std::string& text)
{
	std::size_t count = 0;
	std::size_t i = 0;
	while (i < text.size())
	{
		i += Utf8CodepointLength(static_cast<unsigned char>(text[i]));
		++count;
	}

	return count;
}

std::string TruncateUtf8ToCodepoints(const std::string& text, const std::size_t maxCodepoints)
{
	std::string result;
	std::size_t codepointCount = 0;
	std::size_t i = 0;

	while (i < text.size() && codepointCount < maxCodepoints)
	{
		const std::size_t length = std::min(Utf8CodepointLength(static_cast<unsigned char>(text[i])), text.size() - i);
		result.append(text, i, length);
		i += length;
		++codepointCount;
	}

	return result;
}

std::string ToStemPrefix(const std::string& token)
{
	const std::size_t codepoints = Utf8CodepointCount(token);
	if (codepoints <= MinStemCodepoints)
	{
		return token;
	}

	const std::size_t stemLength = std::max(MinStemCodepoints, codepoints * 2 / 3);
	return TruncateUtf8ToCodepoints(token, stemLength);
}

void AppendTokens(std::vector<std::string>& tokens, const std::string& text)
{
	std::string current;
	for (const char ch : text)
	{
		if (std::isspace(static_cast<unsigned char>(ch)))
		{
			if (!current.empty())
			{
				tokens.push_back(current);
				current.clear();
			}
		}
		else
		{
			current += ch;
		}
	}

	if (!current.empty())
	{
		tokens.push_back(current);
	}
}

std::string SanitizeToken(const std::string& token)
{
	std::string result;
	for (const char ch : token)
	{
		if (ch == '"' || ch == '*' || ch == ':' || ch == '(' || ch == ')' || ch == '^')
		{
			continue;
		}
		result += ch;
	}

	return result;
}

std::string BuildMatchQuery(const SearchCriteria& criteria)
{
	std::vector<std::string> tokens;

	if (criteria.name.has_value())
	{
		AppendTokens(tokens, *criteria.name);
	}
	if (criteria.role.has_value())
	{
		AppendTokens(tokens, *criteria.role);
	}
	if (criteria.location.has_value())
	{
		AppendTokens(tokens, *criteria.location);
	}
	for (const auto& keyword : criteria.keywords)
	{
		AppendTokens(tokens, keyword);
	}

	std::string query;
	for (const auto& token : tokens)
	{
		const std::string sanitized = SanitizeToken(token);
		if (sanitized.empty())
		{
			continue;
		}

		if (!query.empty())
		{
			query += " OR ";
		}
		query += ToStemPrefix(sanitized) + "*";
	}

	return query;
}
} // namespace

SqliteContactRepository::SqliteContactRepository(const std::filesystem::path& databasePath)
	: m_connection(std::make_unique<SqliteConnection>(databasePath))
{
	CreateSchema();
}

void SqliteContactRepository::CreateSchema() const
{
	m_connection->Execute(R"sql(
		CREATE TABLE IF NOT EXISTS contacts (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			canonical_name TEXT NOT NULL DEFAULT '',
			role TEXT NOT NULL DEFAULT '',
			location TEXT NOT NULL DEFAULT '',
			notes TEXT NOT NULL DEFAULT ''
		);
	)sql");

	m_connection->Execute(R"sql(
		CREATE TABLE IF NOT EXISTS contact_channels (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			contact_id INTEGER NOT NULL REFERENCES contacts(id) ON DELETE CASCADE,
			channel_type INTEGER NOT NULL,
			value TEXT NOT NULL
		);
	)sql");

	m_connection->Execute(
		"CREATE INDEX IF NOT EXISTS idx_contact_channels_lookup ON contact_channels(channel_type, value);");

	m_connection->Execute(R"sql(
		CREATE TABLE IF NOT EXISTS contact_sources (
			contact_id INTEGER NOT NULL REFERENCES contacts(id) ON DELETE CASCADE,
			source INTEGER NOT NULL,
			PRIMARY KEY (contact_id, source)
		);
	)sql");

	m_connection->Execute(
		"CREATE VIRTUAL TABLE IF NOT EXISTS contacts_fts USING fts5(canonical_name, role, location, notes);");
}

std::optional<Contact> SqliteContactRepository::FindByChannel(const ContactChannel& channel) const
{
	const SqliteStatement statement(
		m_connection->GetHandle(), "SELECT contact_id FROM contact_channels WHERE channel_type = ? AND value = ? LIMIT 1;");
	statement.BindInt64(1, static_cast<std::int64_t>(channel.type));
	statement.BindText(2, channel.value);

	if (!statement.Step())
	{
		return std::nullopt;
	}

	return LoadContact(statement.ColumnInt64(0));
}

Contact SqliteContactRepository::Upsert(const Contact& contact)
{
	m_connection->Execute("BEGIN;");

	try
	{
		Contact saved = contact;
		saved.id = SaveContactRow(contact);

		ReplaceChannels(saved.id, saved.channels);
		ReplaceSources(saved.id, saved.sources);
		SyncFts(saved);

		m_connection->Execute("COMMIT;");

		return saved;
	}
	catch (...)
	{
		m_connection->Execute("ROLLBACK;");
		throw;
	}
}

std::int64_t SqliteContactRepository::SaveContactRow(const Contact& contact) const
{
	if (contact.id == 0)
	{
		const SqliteStatement statement(
			m_connection->GetHandle(),
			"INSERT INTO contacts (canonical_name, role, location, notes) VALUES (?, ?, ?, ?);");
		statement.BindText(1, contact.canonicalName);
		statement.BindText(2, contact.role);
		statement.BindText(3, contact.location);
		statement.BindText(4, contact.notes);
		statement.Run();

		return m_connection->GetLastInsertRowId();
	}

	const SqliteStatement statement(
		m_connection->GetHandle(),
		"UPDATE contacts SET canonical_name = ?, role = ?, location = ?, notes = ? WHERE id = ?;");
	statement.BindText(1, contact.canonicalName);
	statement.BindText(2, contact.role);
	statement.BindText(3, contact.location);
	statement.BindText(4, contact.notes);
	statement.BindInt64(5, contact.id);
	statement.Run();

	return contact.id;
}

void SqliteContactRepository::ReplaceChannels(
	const std::int64_t contactId,
	const std::vector<ContactChannel>& channels) const
{
	const SqliteStatement deleteStatement(m_connection->GetHandle(), "DELETE FROM contact_channels WHERE contact_id = ?;");
	deleteStatement.BindInt64(1, contactId);
	deleteStatement.Run();

	for (const auto& channel : channels)
	{
		const SqliteStatement insertStatement(
			m_connection->GetHandle(),
			"INSERT INTO contact_channels (contact_id, channel_type, value) VALUES (?, ?, ?);");
		insertStatement.BindInt64(1, contactId);
		insertStatement.BindInt64(2, static_cast<std::int64_t>(channel.type));
		insertStatement.BindText(3, channel.value);
		insertStatement.Run();
	}
}

void SqliteContactRepository::ReplaceSources(
	const std::int64_t contactId,
	const std::vector<ContactSource>& sources) const
{
	const SqliteStatement deleteStatement(m_connection->GetHandle(), "DELETE FROM contact_sources WHERE contact_id = ?;");
	deleteStatement.BindInt64(1, contactId);
	deleteStatement.Run();

	for (const auto& source : sources)
	{
		const SqliteStatement insertStatement(
			m_connection->GetHandle(), "INSERT INTO contact_sources (contact_id, source) VALUES (?, ?);");
		insertStatement.BindInt64(1, contactId);
		insertStatement.BindInt64(2, static_cast<std::int64_t>(source));
		insertStatement.Run();
	}
}

void SqliteContactRepository::SyncFts(const Contact& contact) const
{
	const SqliteStatement deleteStatement(m_connection->GetHandle(), "DELETE FROM contacts_fts WHERE rowid = ?;");
	deleteStatement.BindInt64(1, contact.id);
	deleteStatement.Run();

	const SqliteStatement insertStatement(
		m_connection->GetHandle(),
		"INSERT INTO contacts_fts (rowid, canonical_name, role, location, notes) VALUES (?, ?, ?, ?, ?);");
	insertStatement.BindInt64(1, contact.id);
	insertStatement.BindText(2, contact.canonicalName);
	insertStatement.BindText(3, contact.role);
	insertStatement.BindText(4, contact.location);
	insertStatement.BindText(5, contact.notes);
	insertStatement.Run();
}

Contact SqliteContactRepository::LoadContact(const std::int64_t contactId) const
{
	const SqliteStatement statement(
		m_connection->GetHandle(), "SELECT id, canonical_name, role, location, notes FROM contacts WHERE id = ?;");
	statement.BindInt64(1, contactId);

	if (!statement.Step())
	{
		throw std::runtime_error(
			"[SqliteContactRepository] Контакт " + std::to_string(contactId) + " не найден");
	}

	Contact contact;
	contact.id = statement.ColumnInt64(0);
	contact.canonicalName = statement.ColumnText(1);
	contact.role = statement.ColumnText(2);
	contact.location = statement.ColumnText(3);
	contact.notes = statement.ColumnText(4);
	contact.channels = LoadChannels(contactId);
	contact.sources = LoadSources(contactId);

	return contact;
}

std::vector<ContactChannel> SqliteContactRepository::LoadChannels(const std::int64_t contactId) const
{
	const SqliteStatement statement(
		m_connection->GetHandle(), "SELECT channel_type, value FROM contact_channels WHERE contact_id = ?;");
	statement.BindInt64(1, contactId);

	std::vector<ContactChannel> channels;
	while (statement.Step())
	{
		ContactChannel channel;
		channel.type = static_cast<ChannelType>(statement.ColumnInt64(0));
		channel.value = statement.ColumnText(1);
		channels.push_back(channel);
	}

	return channels;
}

std::vector<ContactSource> SqliteContactRepository::LoadSources(const std::int64_t contactId) const
{
	const SqliteStatement statement(m_connection->GetHandle(), "SELECT source FROM contact_sources WHERE contact_id = ?;");
	statement.BindInt64(1, contactId);

	std::vector<ContactSource> sources;
	while (statement.Step())
	{
		sources.push_back(static_cast<ContactSource>(statement.ColumnInt64(0)));
	}

	return sources;
}

std::vector<SearchResult> SqliteContactRepository::Search(const SearchCriteria& criteria, const std::size_t limit) const
{
	const std::string matchQuery = BuildMatchQuery(criteria);
	if (matchQuery.empty())
	{
		return {};
	}

	const SqliteStatement statement(
		m_connection->GetHandle(),
		"SELECT rowid, bm25(contacts_fts) FROM contacts_fts WHERE contacts_fts MATCH ? "
		"ORDER BY bm25(contacts_fts) LIMIT ?;");
	statement.BindText(1, matchQuery);
	statement.BindInt64(2, static_cast<std::int64_t>(limit));

	std::vector<SearchResult> results;
	while (statement.Step())
	{
		SearchResult result;
		result.contact = LoadContact(statement.ColumnInt64(0));
		result.matchScore = -statement.ColumnDouble(1);
		results.push_back(result);
	}

	return results;
}
