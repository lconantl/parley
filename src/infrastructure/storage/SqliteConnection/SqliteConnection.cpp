#include "SqliteConnection.hpp"

#include <sqlite3.h>

#include <stdexcept>

namespace
{
void AssertIsOpened(const int resultCode, sqlite3* handle, const std::filesystem::path& path)
{
	if (resultCode != SQLITE_OK)
	{
		const std::string message = handle == nullptr ? "неизвестная ошибка" : sqlite3_errmsg(handle);
		throw std::runtime_error(
			"[SqliteConnection] Не удалось открыть базу данных '" + path.string() + "': " + message);
	}
}

void AssertIsExecuted(const int resultCode, sqlite3* handle, const std::string& sql, char* errorMessage)
{
	if (resultCode != SQLITE_OK)
	{
		std::string message = errorMessage != nullptr ? errorMessage : sqlite3_errmsg(handle);
		sqlite3_free(errorMessage);
		throw std::runtime_error("[SqliteConnection] Ошибка выполнения '" + sql + "': " + message);
	}
}
} // namespace

SqliteConnection::SqliteConnection(const std::filesystem::path& path)
{
	const int resultCode = sqlite3_open(path.string().c_str(), &m_handle);
	AssertIsOpened(resultCode, m_handle, path);

	Execute("PRAGMA foreign_keys = ON;");
}

SqliteConnection::~SqliteConnection()
{
	if (m_handle != nullptr)
	{
		sqlite3_close(m_handle);
	}
}

sqlite3* SqliteConnection::GetHandle() const noexcept
{
	return m_handle;
}

void SqliteConnection::Execute(const std::string& sql) const
{
	char* errorMessage = nullptr;
	const int resultCode = sqlite3_exec(m_handle, sql.c_str(), nullptr, nullptr, &errorMessage);
	AssertIsExecuted(resultCode, m_handle, sql, errorMessage);
}

std::int64_t SqliteConnection::GetLastInsertRowId() const
{
	return sqlite3_last_insert_rowid(m_handle);
}
