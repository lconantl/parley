#include "WorkerPool.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
constexpr std::size_t MinThreadCount = 2;
constexpr std::size_t MaxThreadCount = 8;

void AssertIsThreadCountValid(const std::size_t threadCount)
{
	if (threadCount == 0)
	{
		throw std::invalid_argument("Число рабочих потоков должно быть больше нуля");
	}
}

void AssertIsTaskValid(const std::function<void()>& task)
{
	if (!task)
	{
		throw std::invalid_argument("Задача не может быть пустой");
	}
}

void LogTaskFailure(const std::exception& error)
{
	std::cerr << "Ошибка в рабочем потоке: " << error.what() << std::endl;
}
} // namespace

std::size_t WorkerPool::SuggestThreadCount()
{
	const unsigned int hardware = std::thread::hardware_concurrency();
	if (hardware == 0)
	{
		return MinThreadCount;
	}

	return std::min<std::size_t>(std::max<std::size_t>(hardware, MinThreadCount), MaxThreadCount);
}

WorkerPool::WorkerPool(const std::size_t threadCount)
{
	AssertIsThreadCountValid(threadCount);

	m_threads.reserve(threadCount);
	for (std::size_t index = 0; index < threadCount; ++index)
	{
		m_threads.emplace_back([this] { Run(); });
	}
}

WorkerPool::~WorkerPool()
{
	Stop();
}

void WorkerPool::Post(std::function<void()> task)
{
	AssertIsTaskValid(task);

	{
		std::lock_guard lock(m_mutex);
		if (m_stopping)
		{
			return;
		}

		m_tasks.push(std::move(task));
	}

	m_condition.notify_one();
}

void WorkerPool::Stop()
{
	{
		std::lock_guard lock(m_mutex);
		if (m_stopping)
		{
			return;
		}

		m_stopping = true;
	}

	m_condition.notify_all();

	for (auto& thread : m_threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	m_threads.clear();
}

std::size_t WorkerPool::GetQueueSize() const
{
	std::lock_guard lock(m_mutex);

	return m_tasks.size();
}

std::size_t WorkerPool::GetThreadCount() const noexcept
{
	return m_threads.size();
}

bool WorkerPool::TryTakeTask(std::function<void()>& task)
{
	std::unique_lock lock(m_mutex);
	m_condition.wait(lock, [this] { return m_stopping || !m_tasks.empty(); });

	if (m_stopping && m_tasks.empty())
	{
		return false;
	}

	task = std::move(m_tasks.front());
	m_tasks.pop();

	return true;
}

void WorkerPool::Run()
{
	while (true)
	{
		std::function<void()> task;
		if (!TryTakeTask(task))
		{
			return;
		}

		try
		{
			task();
		}
		catch (const std::exception& error)
		{
			LogTaskFailure(error);
		}
	}
}