#include "Statement.hpp"

#include <sqlite3.h>
#include <stdexcept>

namespace
{
void Finalize(sqlite3_stmt* handle)
{
	sqlite3_finalize(handle);
}

void AssertIsSuccessful(int result)
{
	if (result != SQLITE_OK && result != SQLITE_ROW && result != SQLITE_DONE)
	{
		throw std::runtime_error("Не удалось выполнить запрос SQLite, код " +
								 std::to_string(result));
	}
}
}

Statement::Statement(const Database& database, const std::string& sql) : m_handle(nullptr, Finalize)
{
	sqlite3_stmt* statement = nullptr;
	const auto result =
		sqlite3_prepare_v2(database.GetHandle(), sql.c_str(), -1, &statement, nullptr);
	m_handle.reset(statement);
	AssertIsSuccessful(result);
}

Statement::~Statement() = default;

void Statement::Bind(int index, const std::string& value) const
{
	AssertIsSuccessful(sqlite3_bind_text(m_handle.get(), index, value.c_str(),
										 static_cast<int>(value.size()), SQLITE_TRANSIENT));
}

void Statement::Bind(int index, std::int64_t value) const
{
	AssertIsSuccessful(sqlite3_bind_int64(m_handle.get(), index, value));
}

bool Statement::Step() const
{
	const auto result = sqlite3_step(m_handle.get());
	AssertIsSuccessful(result);
	return result == SQLITE_ROW;
}

std::string Statement::Text(int index) const
{
	const auto* value = sqlite3_column_text(m_handle.get(), index);
	return value ? reinterpret_cast<const char*>(value) : "";
}

std::int64_t Statement::Integer(int index) const noexcept
{
	return sqlite3_column_int64(m_handle.get(), index);
}
