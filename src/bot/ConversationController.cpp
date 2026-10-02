#include "ConversationController.hpp"

#include "ui/Keyboard.hpp"
#include "ui/Presenter.hpp"
#include "ui/Text.hpp"
#include <algorithm>
#include <charconv>

namespace
{
std::int64_t ParseNumber(const std::string& text)
{
	std::int64_t value = -1;
	const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
	return result.ec == std::errc{} && result.ptr == text.data() + text.size() ? value : -1;
}

bool IsPrivate(const nlohmann::json& message, std::int64_t userId)
{
	return message.contains("chat") && message["chat"].value("type", "") == "private" &&
		   message["chat"].value("id", std::int64_t{0}) == userId;
}

std::string Command(const std::string& text)
{
	if (!text.starts_with('/'))
	{
		return text;
	}
	return text.substr(0, text.find_first_of(" @"));
}
}

ConversationController::ConversationController(const Config& config, MatchStore& store,
											   ITelegramClient& client)
	: m_config(config), m_store(store), m_messages(client)
{
}

void ConversationController::Handle(const nlohmann::json& update)
{
	if (update.contains("message"))
	{
		HandleMessage(update["message"], update.at("update_id").get<std::int64_t>());
	}
	else if (update.contains("callback_query"))
	{
		HandleCallback(update["callback_query"], update.at("update_id").get<std::int64_t>());
	}
}

void ConversationController::HandleMessage(const nlohmann::json& message, std::int64_t updateId)
{
	if (!message.contains("from"))
	{
		return;
	}
	const auto userId = message["from"].value("id", std::int64_t{0});
	if (!IsPrivate(message, userId))
	{
		return;
	}
	if (!m_config.IsAllowed(userId))
	{
		m_messages.Send(userId, "Доступ закрыт. Ваш Telegram ID: " + std::to_string(userId) +
									". Передайте его владельцу бота.");
		return;
	}
	const auto command = Command(message.value("text", ""));
	if (command == "/start" || command == "/help")
	{
		m_messages.Send(
			userId,
			"<b>Parley · покупатели и активы</b>\n\n"
			"Смотрите пары, сохраняйте подходящие и получайте готовые обращения.\n"
			"❤️ Подходит — сохранить и получить текст.\n👎 Пропустить — следующая пара.\n"
			"↩️ Отменить решение — вернуть последнюю пару.\n\n"
			"Ваш прогресс сохраняется. Сообщения контрагентам вы отправляете сами.",
			Keyboard::Main(), true);
		m_messages.Send(userId, "Пары отсортированы по соответствию запросу.",
						Keyboard::Continue());
	}
	else if (command == "/feed" || command == "🃏 Лента")
	{
		ShowFeed(userId);
	}
	else if (command == "/favorites" || command == "❤️ Избранное")
	{
		ShowFavorites(userId, 0);
	}
	else if (command == "/progress" || command == "📊 Прогресс")
	{
		m_messages.Send(userId, Presenter::Statistics(m_store.GetProgress(userId)),
						Keyboard::Continue(), true);
	}
	else if (command == "/undo")
	{
		const auto last = m_store.LastDecision(userId);
		if (last && m_store.Undo(userId, last->id, updateId))
		{
			ShowCard(userId, *m_store.FindMatch(last->matchId));
		}
		else
		{
			m_messages.Send(userId, "Пока нечего отменять.", Keyboard::Continue());
		}
	}
	else
	{
		m_messages.Send(userId, "Выберите раздел в меню: лента, избранное или прогресс.",
						Keyboard::Main());
	}
}

void ConversationController::ShowFeed(std::int64_t userId, std::int64_t messageId)
{
	const auto match = m_store.Next(userId);
	if (match)
	{
		ShowCard(userId, *match, messageId);
	}
	else
	{
		m_messages.Show(userId, messageId,
						"<b>Все пары просмотрены 🎉</b>\n\n"
						"Подходящие пары и готовые тексты доступны в избранном.",
						Keyboard::Empty(m_store.LastDecision(userId)));
	}
}

void ConversationController::ShowCard(std::int64_t userId, const Match& match,
									  std::int64_t messageId)
{
	const auto decision = m_store.GetDecision(userId, match.id);
	m_messages.Show(userId, messageId,
					Presenter::Card(match, m_store.GetProgress(userId), decision),
					Keyboard::Card(match, decision, m_store.LastDecision(userId)));
}

void ConversationController::ShowFavorites(std::int64_t userId, int offset, std::int64_t messageId)
{
	const auto total = m_store.GetProgress(userId).liked;
	offset = total == 0 ? 0 : std::clamp(offset / 5 * 5, 0, (total - 1) / 5 * 5);
	const auto matches = m_store.Favorites(userId, offset);
	std::string text = "<b>❤️ Избранное · " + std::to_string(total) + "</b>\n\n";
	if (matches.empty())
	{
		text += "Здесь появятся пары, которые вы отметите кнопкой «Подходит».";
	}
	for (const auto& match : matches)
	{
		text += "<b>" + std::to_string(match.score) + "% · " +
				Text::Escape(Text::Shorten(match.asset.title, 80)) + "</b>\n" +
				Text::Escape(Text::Shorten(match.buyer.contact, 60)) + "\n\n";
	}
	if (total > 0)
	{
		text +=
			"Страница " + std::to_string(offset / 5 + 1) + " из " + std::to_string((total + 4) / 5);
	}
	m_messages.Show(userId, messageId, text, Keyboard::Favorites(matches, offset, total));
}

void ConversationController::ShowTexts(std::int64_t userId, const Match& match,
									   std::int64_t messageId)
{
	const auto mode = m_store.GetMode(userId, match);
	m_messages.Show(
		userId, messageId,
		"<b>✉️ Готовые обращения</b>\n\n" + Text::Escape(match.asset.title) + "\n" +
			Presenter::ModeLabel(mode) +
			"\n\nВыберите адресата. Текст придёт отдельно, чтобы его было удобно скопировать.",
		Keyboard::Draft(match, mode));
}

void ConversationController::ShowDraft(std::int64_t userId, const Match& match,
									   const std::string& code)
{
	const auto draft = m_store.GetDraft(match.id, code);
	const auto mode = code.starts_with('A') ? "buy_side" : "sell_side";
	if (!draft.applicable)
	{
		m_messages.Send(userId, "Данных по активу достаточно: уточняющие вопросы не нужны.",
						Keyboard::Draft(match, mode));
		return;
	}
	m_messages.Send(userId, draft.label + "\n\n" + draft.context);
	m_messages.Send(userId, draft.body);
	m_messages.Send(userId, "Обращение выше готово для копирования.\n" + Presenter::ModeLabel(mode),
					Keyboard::Draft(match, mode));
}

void ConversationController::HandleCallback(const nlohmann::json& query, std::int64_t updateId)
{
	if (!query.contains("from") || !query.contains("id"))
	{
		return;
	}
	const auto userId = query["from"].value("id", std::int64_t{0});
	const auto callbackId = query["id"].get<std::string>();
	if (!m_config.IsAllowed(userId) || !query.contains("message") ||
		!IsPrivate(query["message"], userId))
	{
		m_messages.Acknowledge(callbackId, "Недоступно");
		return;
	}
	m_messages.Acknowledge(callbackId);
	const auto messageId = query["message"].value("message_id", std::int64_t{0});
	const auto data = query.value("data", "");
	const auto separator = data.find(':');
	const auto action = data.substr(0, separator);
	const auto argument = separator == std::string::npos ? "" : data.substr(separator + 1);
	if (action == "feed")
	{
		ShowFeed(userId, messageId);
		return;
	}
	if (action == "favorites")
	{
		const auto offset = ParseNumber(argument);
		ShowFavorites(userId, offset >= 0 && offset < 1000000 ? static_cast<int>(offset) : 0,
					  messageId);
		return;
	}
	if (action == "undo")
	{
		const auto restored = m_store.Undo(userId, ParseNumber(argument), updateId);
		if (restored)
		{
			ShowCard(userId, *m_store.FindMatch(*restored), messageId);
		}
		else
		{
			m_messages.Send(userId,
							"Эта отмена уже недоступна. Откройте ленту для текущего решения.",
							Keyboard::Continue());
		}
		return;
	}
	const auto match = m_store.FindMatch(argument);
	if (!match)
	{
		m_messages.Send(userId, "Эта кнопка устарела. Откройте ленту заново.",
						Keyboard::Continue());
		return;
	}
	HandleMatchAction(userId, messageId, updateId, action, *match);
}

void ConversationController::HandleMatchAction(std::int64_t userId, std::int64_t messageId,
											   std::int64_t updateId, const std::string& action,
											   const Match& match)
{
	if (action == "like" || action == "no" || action == "remove")
	{
		const auto decision = action == "like" ? "like" : "dislike";
		const auto changed =
			m_store.Decide(userId, match.id, decision, updateId, action == "remove");
		if (!changed)
		{
			ShowCard(userId, match, messageId);
			return;
		}
		if (action == "like")
		{
			ShowCard(userId, match, messageId);
			ShowDraft(userId, match, m_store.GetMode(userId, match) == "buy_side" ? "A1" : "B1");
		}
		else if (action == "remove")
		{
			ShowFavorites(userId, 0, messageId);
		}
		else
		{
			ShowFeed(userId, messageId);
		}
	}
	else if (action == "card")
	{
		ShowCard(userId, match, messageId);
	}
	else if (action == "details")
	{
		m_messages.Send(userId, match.details, Keyboard::Back(match));
		const auto draft = m_store.GetDraft(match.id, "A1");
		m_messages.Send(userId, "Исходные цитаты и контакты\n\n" + draft.context,
						Keyboard::Back(match));
	}
	else if (action == "texts")
	{
		ShowTexts(userId, match, messageId);
	}
	else if (action == "modes")
	{
		m_messages.Show(userId, messageId,
						"<b>Сценарий обращения</b>\n\n"
						"А — работаем под запрос покупателя.\nБ — предлагаем актив покупателям.\n\n"
						"Текущий: " +
							Presenter::ModeLabel(m_store.GetMode(userId, match)),
						Keyboard::Modes(match));
	}
	else if (action == "buy" || action == "sell")
	{
		m_store.SetMode(userId, match.id, action == "buy" ? "buy_side" : "sell_side");
		ShowTexts(userId, match, messageId);
	}
	else if (action == "A1" || action == "A2" || action == "B1" || action == "B2")
	{
		ShowDraft(userId, match, action);
	}
	else
	{
		m_messages.Send(userId, "Эта кнопка устарела. Откройте ленту заново.",
						Keyboard::Continue());
	}
}
