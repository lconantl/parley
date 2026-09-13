#include "SqliteStatement.hpp"

#include <sqlite3.h>

#include <stdexcept>

namespace
{
void AssertIsPrepared(const int resultCode, sqlite3* database, const std::string& sql)
{
	if (resultCode != SQLITE_OK)
	{
		throw std::runtime_error(
			"[SqliteStatement] Не удалось подготовить запрос '" + sql + "': " + sqlite3_errmsg(database));
	}
}

void AssertIsStepValid(const int resultCode, sqlite3* database)
{
	if (resultCode != SQLITE_ROW && resultCode != SQLITE_DONE)
	{
		throw std::runtime_error(std::string("[SqliteStatement] Ошибка выполнения шага: ") + sqlite3_errmsg(database));
	}
}
} // namespace

SqliteStatement::SqliteStatement(sqlite3* database, const std::string& sql)
	: m_database(database)
{
	const int resultCode = sqlite3_prepare_v2(m_database, sql.c_str(), -1, &m_statement, nullptr);
	AssertIsPrepared(resultCode, m_database, sql);
}

SqliteStatement::~SqliteStatement()
{
	sqlite3_finalize(m_statement);
}

void SqliteStatement::BindInt64(const int index, const std::int64_t value) const
{
	sqlite3_bind_int64(m_statement, index, value);
}

void SqliteStatement::BindText(const int index, const std::string& value) const
{
	sqlite3_bind_text(m_statement, index, value.c_str(), -1, SQLITE_TRANSIENT);
}

bool SqliteStatement::Step() const
{
	const int resultCode = sqlite3_step(m_statement);
	AssertIsStepValid(resultCode, m_database);

	return resultCode == SQLITE_ROW;
}

void SqliteStatement::Run() const
{
	Step();
}

std::int64_t SqliteStatement::ColumnInt64(const int index) const
{
	return sqlite3_column_int64(m_statement, index);
}

double SqliteStatement::ColumnDouble(const int index) const
{
	return sqlite3_column_double(m_statement, index);
}

std::string SqliteStatement::ColumnText(const int index) const
{
	const unsigned char* text = sqlite3_column_text(m_statement, index);
	return text == nullptr ? "" : reinterpret_cast<const char*>(text);
}
