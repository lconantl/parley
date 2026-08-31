#pragma once

#include "common/config/Config.hpp"
#include <memory>
#include <unordered_set>

namespace TgBot
{
class Bot;
class Message;
} // namespace TgBot

class ParleyBot
{
public:
	explicit ParleyBot(const Config& config);
	~ParleyBot();

	void Run() const;

private:
	std::unique_ptr<TgBot::Bot> m_bot;
	Config m_config;
	std::unordered_set<int64_t> m_activeUsers;

	void ProcessMessage(const std::shared_ptr<TgBot::Message>& message);
};