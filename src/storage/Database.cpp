#include "Database.hpp"

#include <sqlite3.h>
#include <stdexcept>

namespace
{
void CloseDatabase(sqlite3* handle)
{
	sqlite3_close_v2(handle);
}

void AssertIsSuccessful(int result)
{
	if (result != SQLITE_OK)
	{
		throw std::runtime_error("Ошибка SQLite, код " + std::to_string(result));
	}
}
}

Database::Database(const std::filesystem::path& path) : m_handle(nullptr, CloseDatabase)
{
	sqlite3* handle = nullptr;
	const auto encoded = path.u8string();
	const auto result = sqlite3_open_v2(reinterpret_cast<const char*>(encoded.c_str()), &handle,
										SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, nullptr);
	m_handle.reset(handle);
	AssertIsSuccessful(result);
	AssertIsSuccessful(sqlite3_busy_timeout(handle, 5000));
	Execute("PRAGMA foreign_keys=ON");
	Execute("PRAGMA journal_mode=DELETE");
	Execute("PRAGMA synchronous=FULL");
}

Database::~Database() = default;

sqlite3* Database::GetHandle() const noexcept
{
	return m_handle.get();
}

void Database::Execute(const std::string& sql) const
{
	AssertIsSuccessful(sqlite3_exec(m_handle.get(), sql.c_str(), nullptr, nullptr, nullptr));
}

int Database::GetChanges() const noexcept
{
	return sqlite3_changes(m_handle.get());
}
