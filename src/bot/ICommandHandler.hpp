#pragma once

#include "MessageManager.hpp"
#include "ParsedMessage.hpp"

#include <string>

struct CommandContext
{
	ParsedMessage message;
	MessageManager* messages = nullptr;
};

class ICommandHandler
{
public:
	virtual ~ICommandHandler() = default;

	virtual std::string GetName() const = 0;
	virtual std::string GetDescription() const = 0;
	virtual void Execute(const CommandContext& context) = 0;
};