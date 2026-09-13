#pragma once

#include "infrastructure/pool/WorkerPool.hpp"

#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <unordered_map>

class ChatSequencer
{
public:
	explicit ChatSequencer(WorkerPool& workers);

	void Enqueue(std::int64_t chatId, std::function<void()> job);

private:
	void RunNext(std::int64_t chatId);

	WorkerPool& m_workers;
	std::mutex m_mutex;
	std::unordered_map<std::int64_t, std::deque<std::function<void()>>> m_pending;
	std::unordered_map<std::int64_t, bool> m_busy;
};
