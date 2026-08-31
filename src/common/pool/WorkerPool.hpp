#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class WorkerPool
{
public:
	explicit WorkerPool(std::size_t threadCount);
	~WorkerPool();

	WorkerPool(const WorkerPool&) = delete;
	WorkerPool& operator=(const WorkerPool&) = delete;

	void Post(std::function<void()> task);
	void Stop();

	std::size_t GetQueueSize() const;
	std::size_t GetThreadCount() const noexcept;

	static std::size_t SuggestThreadCount();

private:
	void Run();
	bool TryTakeTask(std::function<void()>& task);

	std::vector<std::thread> m_threads;
	std::queue<std::function<void()>> m_tasks;
	mutable std::mutex m_mutex;
	std::condition_variable m_condition;
	bool m_stopping = false;
};