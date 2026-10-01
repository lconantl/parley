#include "Keyboard.hpp"

#include "Text.hpp"

namespace
{
using Json = nlohmann::json;

Json Button(const std::string& title, const std::string& data)
{
	return {{"text", title}, {"callback_data", data}};
}

Json Wrap(Json rows)
{
	return {{"inline_keyboard", std::move(rows)}};
}
}

nlohmann::json Keyboard::Main()
{
	return {{"keyboard",
			 {{{{"text", "🃏 Лента"}}, {{"text", "❤️ Избранное"}}, {{"text", "📊 Прогресс"}}}}},
			{"resize_keyboard", true},
			{"is_persistent", true}};
}

nlohmann::json Keyboard::Continue()
{
	return Wrap({{Button("Начать подбор →", "feed")}});
}

nlohmann::json Keyboard::Card(const Match& match, const std::string& decision,
							  const std::optional<DecisionEvent>& last)
{
	Json rows = Json::array();
	if (decision.empty())
	{
		rows.push_back(
			{Button("👎 Пропустить", "no:" + match.id), Button("❤️ Подходит", "like:" + match.id)});
	}
	else if (decision == "like")
	{
		rows.push_back({Button("✉️ Тексты", "texts:" + match.id),
						Button("Убрать из избранного", "remove:" + match.id)});
	}
	Json secondary = {Button("🔎 Подробнее", "details:" + match.id)};
	if (last)
	{
		secondary.push_back(Button("↩️ Отменить решение", "undo:" + std::to_string(last->id)));
	}
	rows.push_back(secondary);
	if (!decision.empty())
	{
		rows.push_back(
			{Button("🃏 Продолжить ленту", "feed"), Button("❤️ Избранное", "favorites:0")});
	}
	return Wrap(rows);
}

nlohmann::json Keyboard::Draft(const Match& match, const std::string& mode)
{
	const auto prefix = mode == "buy_side" ? "A" : "B";
	return Wrap({{Button("✉️ Покупателю", std::string(prefix) + "1:" + match.id),
				  Button("✉️ Продавцу", std::string(prefix) + "2:" + match.id)},
				 {Button("🔄 Сценарий обращения", "modes:" + match.id)},
				 {Button("← К паре", "card:" + match.id), Button("Следующая пара →", "feed")}});
}

nlohmann::json Keyboard::Modes(const Match& match)
{
	return Wrap({{Button("Под запрос покупателя · А", "buy:" + match.id)},
				 {Button("Под предложение продавца · Б", "sell:" + match.id)},
				 {Button("← К текстам", "texts:" + match.id)}});
}

nlohmann::json Keyboard::Back(const Match& match)
{
	return Wrap({{Button("← К карточке", "card:" + match.id)}});
}

nlohmann::json Keyboard::Favorites(const std::vector<Match>& matches, int offset, int total)
{
	Json rows = Json::array();
	for (const auto& match : matches)
	{
		rows.push_back(
			{Button(std::to_string(match.score) + "% · " + Text::Shorten(match.asset.title, 44),
					"card:" + match.id)});
	}
	Json navigation = Json::array();
	if (offset > 0)
	{
		navigation.push_back(Button("← Назад", "favorites:" + std::to_string(offset - 5)));
	}
	if (offset + 5 < total)
	{
		navigation.push_back(Button("Далее →", "favorites:" + std::to_string(offset + 5)));
	}
	if (!navigation.empty())
	{
		rows.push_back(navigation);
	}
	rows.push_back({Button("🃏 Продолжить ленту", "feed")});
	return Wrap(rows);
}

nlohmann::json Keyboard::Empty(const std::optional<DecisionEvent>& last)
{
	Json rows = {{Button("❤️ Избранное", "favorites:0")}};
	if (last)
	{
		rows.push_back({Button("↩️ Отменить решение", "undo:" + std::to_string(last->id))});
	}
	return Wrap(rows);
}
