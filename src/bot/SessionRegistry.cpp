#include "SessionRegistry.hpp"

bool SessionRegistry::TryBegin(const std::int64_t userId)
{
	std::lock_guard lock(m_mutex);

	return m_activeUsers.insert(userId).second;
}

void SessionRegistry::End(const std::int64_t userId)
{
	std::lock_guard lock(m_mutex);
	m_activeUsers.erase(userId);
}

bool SessionRegistry::IsActive(const std::int64_t userId) const
{
	std::lock_guard lock(m_mutex);

	return m_activeUsers.contains(userId);
}

SessionLock::SessionLock(SessionRegistry& registry, const std::int64_t userId)
	: m_registry(registry)
	, m_userId(userId)
	, m_acquired(registry.TryBegin(userId))
{
}

SessionLock::~SessionLock()
{
	if (m_acquired)
	{
		m_registry.End(m_userId);
	}
}

bool SessionLock::IsAcquired() const noexcept
{
	return m_acquired;
}