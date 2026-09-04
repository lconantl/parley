#pragma once

// Чисто декоративные цены для кнопок выбора материала. Реальной оплаты нет —
// значения нигде не сохраняются и не влияют на генерацию документов.
struct PriceQuote
{
	int report = 0;
	int presentation = 0;
	int onePager = 0;
};

namespace PriceLabeler
{
PriceQuote RollPrices();
int AllInclusivePrice(const PriceQuote& prices);
} // namespace PriceLabeler
