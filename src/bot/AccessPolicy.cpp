#include "AccessPolicy.hpp"
#include <algorithm>
#include <utility>

AccessPolicy::AccessPolicy(std::vector<std::int64_t> allowedUsers)
	: m_allowedUsers(std::move(allowedUsers))
{
}

bool AccessPolicy::IsOpenForEveryone() const noexcept
{
	return m_allowedUsers.empty();
}

bool AccessPolicy::IsAllowed(const std::int64_t userId) const
{
	if (IsOpenForEveryone())
	{
		return true;
	}

	return std::ranges::find(m_allowedUsers, userId)
		!= m_allowedUsers.end();
}