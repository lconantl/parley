#pragma once

#include <cstdint>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

class SqliteStatement
{
public:
	SqliteStatement(sqlite3* database, const std::string& sql);
	~SqliteStatement();

	SqliteStatement(const SqliteStatement&) = delete;
	SqliteStatement& operator=(const SqliteStatement&) = delete;

	void BindInt64(int index, std::int64_t value) const;
	void BindText(int index, const std::string& value) const;

	bool Step() const;
	void Run() const;

	std::int64_t ColumnInt64(int index) const;
	double ColumnDouble(int index) const;
	std::string ColumnText(int index) const;

private:
	sqlite3* m_database;
	sqlite3_stmt* m_statement = nullptr;
};
