#pragma once

#include <cstdint>
#include <filesystem>

class InstanceLock
{
  public:
	explicit InstanceLock(const std::filesystem::path& databasePath);
	~InstanceLock() noexcept;
	InstanceLock(const InstanceLock&) = delete;
	InstanceLock& operator=(const InstanceLock&) = delete;

  private:
	std::intptr_t m_handle;
};
