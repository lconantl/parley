#include "StubCommandHandler.hpp"

#include <stdexcept>
#include <utility>

namespace
{
void AssertIsNameValid(const std::string& name)
{
	if (name.empty())
	{
		throw std::invalid_argument("Имя команды не может быть пустым");
	}
}
} // namespace

StubCommandHandler::StubCommandHandler(
	std::string name,
	std::string description,
	std::string replyText)
	: m_name(std::move(name))
	, m_description(std::move(description))
	, m_replyText(std::move(replyText))
{
	AssertIsNameValid(m_name);
}

std::string StubCommandHandler::GetName() const
{
	return m_name;
}

std::string StubCommandHandler::GetDescription() const
{
	return m_description;
}

void StubCommandHandler::Execute(const CommandContext& context)
{
	context.messages->SendText(context.message.chatId, m_replyText);
}