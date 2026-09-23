#include "Transaction.hpp"

Transaction::Transaction(const Database& database) : m_database(database)
{
	m_database.Execute("BEGIN IMMEDIATE");
}

Transaction::~Transaction() noexcept
{
	if (!m_committed)
	{
		try
		{
			m_database.Execute("ROLLBACK");
		}
		catch (...)
		{
		}
	}
}

void Transaction::Commit()
{
	m_database.Execute("COMMIT");
	m_committed = true;
}
