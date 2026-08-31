#include "CommandRouter.hpp"
#include <stdexcept>
#include <utility>

namespace
{
void AssertIsHandlerValid(const std::shared_ptr<ICommandHandler>& handler)
{
	if (handler == nullptr)
	{
		throw std::invalid_argument("Обработчик команды не может быть пустым");
	}
}

void AssertIsNameFree(const bool free, const std::string& name)
{
	if (!free)
	{
		throw std::invalid_argument("Команда уже зарегистрирована: " + name);
	}
}

void AssertIsContextValid(const CommandContext& context)
{
	if (context.messages == nullptr)
	{
		throw std::invalid_argument("Контекст команды не содержит отправителя сообщений");
	}
}
} // namespace

void CommandRouter::Register(std::shared_ptr<ICommandHandler> handler)
{
	AssertIsHandlerValid(handler);
	AssertIsNameFree(!Has(handler->GetName()), handler->GetName());

	m_handlers.push_back(std::move(handler));
}

void CommandRouter::SetFallback(std::shared_ptr<ICommandHandler> handler)
{
	AssertIsHandlerValid(handler);
	m_fallback = std::move(handler);
}

void CommandRouter::SetUnknownCommandText(std::string text)
{
	m_unknownCommandText = std::move(text);
}

std::shared_ptr<ICommandHandler> CommandRouter::Find(const std::string& command) const
{
	for (const auto& handler : m_handlers)
	{
		if (handler->GetName() == command)
		{
			return handler;
		}
	}

	return nullptr;
}

bool CommandRouter::Has(const std::string& command) const
{
	return Find(command) != nullptr;
}

std::vector<CommandInfo> CommandRouter::ListCommands() const
{
	std::vector<CommandInfo> commands;
	commands.reserve(m_handlers.size());

	for (const auto& handler : m_handlers)
	{
		commands.push_back({handler->GetName(), handler->GetDescription()});
	}

	return commands;
}

void CommandRouter::Dispatch(const CommandContext& context) const
{
	AssertIsContextValid(context);

	if (context.message.command.empty())
	{
		if (m_fallback != nullptr)
		{
			m_fallback->Execute(context);
		}

		return;
	}

	const auto handler = Find(context.message.command);
	if (handler == nullptr)
	{
		context.messages->SendText(context.message.chatId, m_unknownCommandText);
		return;
	}

	handler->Execute(context);
}