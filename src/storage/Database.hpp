#pragma once

#include <filesystem>
#include <memory>
#include <string>

struct sqlite3;

class Database
{
  public:
	explicit Database(const std::filesystem::path& path);
	~Database();
	Database(const Database&) = delete;
	Database& operator=(const Database&) = delete;
	sqlite3* GetHandle() const noexcept;
	void Execute(const std::string& sql) const;
	int GetChanges() const noexcept;

  private:
	std::unique_ptr<sqlite3, void (*)(sqlite3*)> m_handle;
};
