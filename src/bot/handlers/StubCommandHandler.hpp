#pragma once

#include "bot/ICommandHandler.hpp"

class StubCommandHandler : public ICommandHandler
{
public:
	StubCommandHandler(std::string name, std::string description, std::string replyText);

	std::string GetName() const override;
	std::string GetDescription() const override;
	void Execute(const CommandContext& context) override;

private:
	std::string m_name;
	std::string m_description;
	std::string m_replyText;
};