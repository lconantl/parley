#include "StartCommandHandler.hpp"

namespace
{
constexpr auto CommandName = "start";
constexpr auto CommandDescription = "Описание бота";

std::string BuildWelcomeText()
{
	return "Привет, я Parley — бот для анализа российских компаний.\n"
		   "\n"
		   "Пришлите ИНН организации, и я соберу данные из реестров, посчитаю "
		   "экономические показатели, а пробелы закрою оценкой по открытым источникам.\n"
		   "\n"
		   "Команды:\n"
		   "/report ИНН — отчет по компании\n"
		   "/pres ИНН — презентация по компании\n"
		   "/pay — оплата анализа";
}
} // namespace

std::string StartCommandHandler::GetName() const
{
	return CommandName;
}

std::string StartCommandHandler::GetDescription() const
{
	return CommandDescription;
}

void StartCommandHandler::Execute(const CommandContext& context)
{
	context.messages->SendText(context.message.chatId, BuildWelcomeText());
}