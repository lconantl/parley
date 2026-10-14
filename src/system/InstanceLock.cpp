#include "InstanceLock.hpp"

#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace
{
void AssertIsLocked(bool locked)
{
	if (!locked)
	{
		throw std::runtime_error(
			"База уже используется другим экземпляром бота или недоступна для записи");
	}
}
}

InstanceLock::InstanceLock(const std::filesystem::path& databasePath) : m_handle(-1)
{
	auto path = databasePath;
	path += ".lock";
#ifdef _WIN32
	const auto handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
									OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	AssertIsLocked(handle != INVALID_HANDLE_VALUE);
	m_handle = reinterpret_cast<std::intptr_t>(handle);
#else
	const auto handle = open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
	AssertIsLocked(handle >= 0);
	if (flock(handle, LOCK_EX | LOCK_NB) != 0)
	{
		close(handle);
		AssertIsLocked(false);
	}
	m_handle = handle;
#endif
}

InstanceLock::~InstanceLock() noexcept
{
#ifdef _WIN32
	CloseHandle(reinterpret_cast<HANDLE>(m_handle));
#else
	flock(static_cast<int>(m_handle), LOCK_UN);
	close(static_cast<int>(m_handle));
#endif
}
