#pragma once

#include "Database.hpp"

class Transaction
{
  public:
	explicit Transaction(const Database& database);
	~Transaction() noexcept;
	Transaction(const Transaction&) = delete;
	Transaction& operator=(const Transaction&) = delete;
	void Commit();

  private:
	const Database& m_database;
	bool m_committed = false;
};
