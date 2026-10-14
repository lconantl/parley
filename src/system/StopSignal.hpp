#pragma once

#include <chrono>

namespace StopSignal
{
void Install();
bool IsRequested() noexcept;
void Wait(std::chrono::milliseconds duration);
}
