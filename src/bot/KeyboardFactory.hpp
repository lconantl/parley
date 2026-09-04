#pragma once

#include "PriceLabeler.hpp"

#include <tgbot/tgbot.h>

namespace KeyboardFactory
{
TgBot::InlineKeyboardMarkup::Ptr DocumentTypeKeyboard(const PriceQuote& prices);
TgBot::InlineKeyboardMarkup::Ptr AnonymityKeyboard();
TgBot::InlineKeyboardMarkup::Ptr Empty();
} // namespace KeyboardFactory
