#pragma once

#include <cstdint>
#include <vector>

class AccessPolicy
{
public:
	explicit AccessPolicy(std::vector<std::int64_t> allowedUsers);

	bool IsAllowed(std::int64_t userId) const;
	bool IsOpenForEveryone() const noexcept;

private:
	std::vector<std::int64_t> m_allowedUsers;
};