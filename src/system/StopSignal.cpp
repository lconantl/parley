#include "StopSignal.hpp"

#include <algorithm>
#include <csignal>
#include <thread>

namespace
{
volatile std::sig_atomic_t g_stopRequested = 0;

void HandleSignal(int)
{
	g_stopRequested = 1;
}
}

void StopSignal::Install()
{
	std::signal(SIGINT, HandleSignal);
	std::signal(SIGTERM, HandleSignal);
}

bool StopSignal::IsRequested() noexcept
{
	return g_stopRequested != 0;
}

void StopSignal::Wait(std::chrono::milliseconds duration)
{
	const auto end = std::chrono::steady_clock::now() + duration;
	while (!IsRequested() && std::chrono::steady_clock::now() < end)
	{
		std::this_thread::sleep_until(
			std::min(end, std::chrono::steady_clock::now() + std::chrono::milliseconds(100)));
	}
}
