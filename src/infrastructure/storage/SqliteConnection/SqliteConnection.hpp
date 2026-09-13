#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

struct sqlite3;

class SqliteConnection
{
public:
	explicit SqliteConnection(const std::filesystem::path& path);
	~SqliteConnection();

	SqliteConnection(const SqliteConnection&) = delete;
	SqliteConnection& operator=(const SqliteConnection&) = delete;

	sqlite3* GetHandle() const noexcept;
	void Execute(const std::string& sql) const;
	std::int64_t GetLastInsertRowId() const;

private:
	sqlite3* m_handle = nullptr;
};
