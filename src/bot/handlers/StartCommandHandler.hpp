#pragma once

#include "bot/ICommandHandler.hpp"

class StartCommandHandler : public ICommandHandler
{
public:
	std::string GetName() const override;
	std::string GetDescription() const override;
	void Execute(const CommandContext& context) override;
};