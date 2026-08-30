#pragma once

#include <memory>
#include <string>

namespace TgBot
{
class Bot;
}

class ParleyBot
{
public:
	explicit ParleyBot(const std::string& token);
	~ParleyBot();

	void Run() const;

private:
	std::unique_ptr<TgBot::Bot> m_bot;
};
