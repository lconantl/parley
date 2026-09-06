#pragma once

#include "PriceLabeler.hpp"

#include <tgbot/tgbot.h>

namespace KeyboardFactory
{
inline constexpr auto StartButtonText = "Запустить";

TgBot::InlineKeyboardMarkup::Ptr DocumentTypeKeyboard(const PriceQuote& prices);
TgBot::InlineKeyboardMarkup::Ptr AnonymityKeyboard();
TgBot::InlineKeyboardMarkup::Ptr Empty();
TgBot::ReplyKeyboardMarkup::Ptr StartKeyboard();
} // namespace KeyboardFactory
