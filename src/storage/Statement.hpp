#pragma once

#include "Database.hpp"
#include <cstdint>
#include <memory>
#include <string>

struct sqlite3_stmt;

class Statement
{
  public:
	Statement(const Database& database, const std::string& sql);
	~Statement();
	Statement(const Statement&) = delete;
	Statement& operator=(const Statement&) = delete;
	void Bind(int index, const std::string& value) const;
	void Bind(int index, std::int64_t value) const;
	bool Step() const;
	std::string Text(int index) const;
	std::int64_t Integer(int index) const noexcept;

  private:
	std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)> m_handle;
};
