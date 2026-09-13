#include "ChatSequencer.hpp"

#include <stdexcept>
#include <utility>

namespace
{
void AssertIsJobValid(const std::function<void()>& job)
{
	if (!job)
	{
		throw std::invalid_argument("Задача не может быть пустой");
	}
}
} // namespace

ChatSequencer::ChatSequencer(WorkerPool& workers)
	: m_workers(workers)
{
}

void ChatSequencer::Enqueue(const std::int64_t chatId, std::function<void()> job)
{
	AssertIsJobValid(job);

	bool shouldStart = false;
	{
		std::lock_guard lock(m_mutex);
		bool& busy = m_busy[chatId];
		if (busy)
		{
			m_pending[chatId].push_back(std::move(job));
		}
		else
		{
			busy = true;
			shouldStart = true;
		}
	}

	if (shouldStart)
	{
		m_workers.Post([this, chatId, job = std::move(job)] {
			job();
			RunNext(chatId);
		});
	}
}

void ChatSequencer::RunNext(const std::int64_t chatId)
{
	std::function<void()> next;
	{
		std::lock_guard lock(m_mutex);
		std::deque<std::function<void()>>& queue = m_pending[chatId];
		if (queue.empty())
		{
			m_busy[chatId] = false;
			return;
		}

		next = std::move(queue.front());
		queue.pop_front();
	}

	m_workers.Post([this, chatId, next = std::move(next)] {
		next();
		RunNext(chatId);
	});
}
