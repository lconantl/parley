#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_set>

class SessionRegistry
{
public:
	bool TryBegin(std::int64_t userId);
	void End(std::int64_t userId);
	bool IsActive(std::int64_t userId) const;

private:
	mutable std::mutex m_mutex;
	std::unordered_set<std::int64_t> m_activeUsers;
};

class SessionLock
{
public:
	SessionLock(SessionRegistry& registry, std::int64_t userId);
	~SessionLock();

	SessionLock(const SessionLock&) = delete;
	SessionLock& operator=(const SessionLock&) = delete;

	bool IsAcquired() const noexcept;

private:
	SessionRegistry& m_registry;
	std::int64_t m_userId;
	bool m_acquired;
};