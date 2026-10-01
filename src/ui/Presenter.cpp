#include "Presenter.hpp"

#include "Text.hpp"
#include <algorithm>

namespace
{
std::string ListingText(const Listing& listing)
{
	return Text::Escape(Text::Shorten(listing.contact, 90)) + "\n" +
		   Text::Escape(Text::Shorten(listing.summary, 390)) + "\n" + "<a href=\"" +
		   Text::Escape(listing.url) + "\">" + Text::Date(listing.date) + " · источник</a>";
}
}

std::string Presenter::Card(const Match& match, const Progress& progress,
							const std::string& decision)
{
	std::string text = "<b>Пара " + std::to_string(match.rank) + " из " +
					   std::to_string(progress.total) + " · Соответствие " +
					   std::to_string(match.score) + "%</b>\n";
	if (decision == "like")
	{
		text += "❤️ В избранном\n";
	}
	else if (!decision.empty())
	{
		text += "👎 Пропущена\n";
	}
	text += "\n👤 <b>Покупатель</b>\n" + ListingText(match.buyer);
	text += "\n\n🏭 <b>Актив</b>\n" + ListingText(match.asset);
	text += "\n\n<b>Почему подходят</b>\n";
	for (std::size_t index = 0; index < std::min<std::size_t>(2, match.reasons.size()); ++index)
	{
		text += "• " + Text::Escape(Text::Shorten(match.reasons[index], 240)) + "\n";
	}
	if (!match.gaps.empty())
	{
		text +=
			"\n<b>Что уточнить</b>\n" + Text::Escape(Text::Shorten(match.gaps.front(), 250)) + "\n";
	}
	text += "\n<b>⚠️ Архивные публикации. Актуальность нужно подтвердить.</b>\n";
	text += "<i>Процент — соответствие запросу, не вероятность сделки.</i>\n\n";
	text += "Просмотрено " + std::to_string(progress.liked + progress.skipped) + "/" +
			std::to_string(progress.total) + " · ❤️ " + std::to_string(progress.liked);
	return text;
}

std::string Presenter::Statistics(const Progress& progress)
{
	return "<b>📊 Ваш прогресс</b>\n\nПросмотрено: " +
		   std::to_string(progress.liked + progress.skipped) + " из " +
		   std::to_string(progress.total) + "\n❤️ Подходит: " + std::to_string(progress.liked) +
		   "\n👎 Пропущено: " + std::to_string(progress.skipped) +
		   "\n🃏 Осталось: " + std::to_string(progress.total - progress.liked - progress.skipped) +
		   "\n\nВаши решения сохранены. Можно продолжить в любой момент.";
}

std::string Presenter::ModeLabel(const std::string& mode)
{
	return mode == "buy_side" ? "Под запрос покупателя · А1/А2"
							  : "Под предложение продавца · Б1/Б2";
}
