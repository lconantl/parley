#include "KeyboardFactory.hpp"

#include <memory>

namespace
{
TgBot::InlineKeyboardButton::Ptr MakeButton(const std::string& text, const std::string& callbackData)
{
	auto button = std::make_shared<TgBot::InlineKeyboardButton>();
	button->text = text;
	button->callbackData = callbackData;

	return button;
}

std::string PriceSuffix(const int price)
{
	return " (" + std::to_string(price) + " ₽)";
}
} // namespace

TgBot::InlineKeyboardMarkup::Ptr KeyboardFactory::DocumentTypeKeyboard(const PriceQuote& prices)
{
	auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();

	keyboard->inlineKeyboard = {
		{MakeButton("Презентация" + PriceSuffix(prices.presentation), "dt:pres")},
		{MakeButton("Одностраничник" + PriceSuffix(prices.onePager), "dt:landing")},
		{MakeButton("Отчёт" + PriceSuffix(prices.report), "dt:report")},
		{MakeButton("Всё сразу" + PriceSuffix(PriceLabeler::AllInclusivePrice(prices)), "dt:all")}};

	return keyboard;
}

TgBot::InlineKeyboardMarkup::Ptr KeyboardFactory::AnonymityKeyboard()
{
	auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();

	keyboard->inlineKeyboard = {
		{MakeButton("Анонимно", "an:1"), MakeButton("Не анонимно", "an:0")}};

	return keyboard;
}

TgBot::InlineKeyboardMarkup::Ptr KeyboardFactory::Empty()
{
	return std::make_shared<TgBot::InlineKeyboardMarkup>();
}

TgBot::ReplyKeyboardMarkup::Ptr KeyboardFactory::StartKeyboard()
{
	auto button = std::make_shared<TgBot::KeyboardButton>();
	button->text = StartButtonText;

	auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
	keyboard->keyboard = {{button}};
	keyboard->resizeKeyboard = true;
	keyboard->oneTimeKeyboard = false;
	keyboard->isPersistent = true;
	keyboard->inputFieldPlaceholder = "Или пришлите ИНН";

	return keyboard;
}