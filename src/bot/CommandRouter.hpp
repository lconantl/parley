#pragma once

#include "ICommandHandler.hpp"

#include <memory>
#include <string>
#include <vector>

struct CommandInfo
{
	std::string name;
	std::string description;
};

class CommandRouter
{
public:
	void Register(std::shared_ptr<ICommandHandler> handler);
	void SetFallback(std::shared_ptr<ICommandHandler> handler);
	void SetUnknownCommandText(std::string text);

	bool Has(const std::string& command) const;
	std::vector<CommandInfo> ListCommands() const;
	void Dispatch(const CommandContext& context) const;

private:
	std::shared_ptr<ICommandHandler> Find(const std::string& command) const;

	std::vector<std::shared_ptr<ICommandHandler>> m_handlers;
	std::shared_ptr<ICommandHandler> m_fallback;
	std::string m_unknownCommandText = "Не знаю такую команду. Наберите /start";
};