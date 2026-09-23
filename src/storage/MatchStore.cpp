#include "MatchStore.hpp"

#include "Statement.hpp"
#include "Transaction.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace
{
void AssertIsValid(bool condition, const std::string& message)
{
	if (!condition)
	{
		throw std::runtime_error(message);
	}
}

int Count(const Database& database, const std::string& sql)
{
	Statement query(database, sql);
	AssertIsValid(query.Step(), "SQLite не вернула результат проверки");
	return static_cast<int>(query.Integer(0));
}

Listing ReadListing(const Statement& row, int start)
{
	return {row.Text(start), row.Text(start + 1), row.Text(start + 2), row.Text(start + 3),
			row.Text(start + 4)};
}

Match ReadMatch(const Statement& row)
{
	Match match;
	match.id = row.Text(0);
	match.rank = static_cast<int>(row.Integer(1));
	match.score = static_cast<int>(row.Integer(2));
	match.suggestedMode = row.Text(3);
	match.buyer = ReadListing(row, 4);
	match.asset = ReadListing(row, 9);
	match.reasons = nlohmann::json::parse(row.Text(14)).get<std::vector<std::string>>();
	match.gaps = nlohmann::json::parse(row.Text(15)).get<std::vector<std::string>>();
	match.details = row.Text(16);
	return match;
}

std::string SelectMatches()
{
	return "SELECT m.id,m.rank,m.score,m.suggested_mode,"
		   "b.title,b.summary,b.contact_name,b.published_at,bs.source_url,"
		   "a.title,a.summary,a.contact_name,a.published_at,asrc.source_url,"
		   "m.reasons_json,m.gaps_json,m.feed_text FROM matches m "
		   "JOIN cards b ON b.id=m.buyer_card_id JOIN cards a ON a.id=m.asset_card_id "
		   "JOIN source_messages bs ON bs.chat_id=b.primary_chat_id "
		   "AND bs.message_id=b.primary_message_id "
		   "JOIN source_messages asrc ON asrc.chat_id=a.primary_chat_id "
		   "AND asrc.message_id=a.primary_message_id ";
}

void SaveDecision(const Database& database, std::int64_t userId, const std::string& matchId,
				  const std::string& decision)
{
	Statement query(database,
					"INSERT INTO review_decisions(reviewer_id,match_id,decision) "
					"VALUES(?,?,?) ON CONFLICT(reviewer_id,match_id) DO UPDATE SET "
					"decision=excluded.decision,reviewed_at=strftime('%Y-%m-%dT%H:%M:%fZ','now')");
	query.Bind(1, std::to_string(userId));
	query.Bind(2, matchId);
	query.Bind(3, decision);
	query.Step();
}
}

MatchStore::MatchStore(const std::filesystem::path& path) : m_database(path)
{
}

void MatchStore::Validate() const
{
	AssertIsValid(Count(m_database, "PRAGMA user_version") == 1, "Неизвестная версия базы пар");
	Statement integrity(m_database, "PRAGMA quick_check");
	AssertIsValid(integrity.Step() && integrity.Text(0) == "ok", "Нарушена целостность базы");
	Statement foreignKeys(m_database, "PRAGMA foreign_key_check");
	AssertIsValid(!foreignKeys.Step(), "Нарушены связи в базе пар");
	AssertIsValid(Count(m_database, "SELECT count(*) FROM matches") > 0, "В базе нет пар");
	AssertIsValid(Count(m_database,
						"SELECT count(*) FROM matches m WHERE "
						"(SELECT count(*) FROM drafts d WHERE d.match_id=m.id)<>4") == 0,
				  "Не для всех пар подготовлены четыре текста");
	Statement matches(m_database, SelectMatches() + "ORDER BY m.rank");
	while (matches.Step())
	{
		const auto match = ReadMatch(matches);
		for (const auto* code : {"A1", "A2", "B1", "B2"})
		{
			AssertIsValid(!GetDraft(match.id, code).body.empty(), "В базе найден пустой текст");
		}
	}
}

void MatchStore::Migrate()
{
	Transaction transaction(m_database);
	m_database.Execute(
		"CREATE TABLE IF NOT EXISTS bot_history("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,user_id TEXT NOT NULL,"
		"match_id TEXT NOT NULL REFERENCES matches(id),previous_decision TEXT NOT NULL,"
		"decision TEXT NOT NULL,update_id INTEGER NOT NULL UNIQUE,undone INTEGER NOT NULL DEFAULT "
		"0)");
	m_database.Execute("CREATE INDEX IF NOT EXISTS bot_history_user_idx "
					   "ON bot_history(user_id,undone,id DESC)");
	m_database.Execute("CREATE TABLE IF NOT EXISTS bot_modes(user_id TEXT NOT NULL,"
					   "match_id TEXT NOT NULL REFERENCES matches(id),mode TEXT NOT NULL,"
					   "PRIMARY KEY(user_id,match_id))");
	m_database.Execute("CREATE TABLE IF NOT EXISTS bot_offsets(bot_id TEXT PRIMARY KEY,"
					   "next_update_id INTEGER NOT NULL)");
	m_database.Execute(
		"CREATE TABLE IF NOT EXISTS bot_undo_updates(update_id INTEGER PRIMARY KEY)");
	transaction.Commit();
}

std::optional<Match> MatchStore::FindMatch(const std::string& matchId) const
{
	Statement query(m_database, SelectMatches() + "WHERE m.id=?");
	query.Bind(1, matchId);
	return query.Step() ? std::optional(ReadMatch(query)) : std::nullopt;
}

std::optional<Match> MatchStore::Next(std::int64_t userId) const
{
	Statement query(m_database,
					SelectMatches() +
						"WHERE NOT EXISTS(SELECT 1 FROM review_decisions d "
						"WHERE d.match_id=m.id AND d.reviewer_id=?) ORDER BY m.rank LIMIT 1");
	query.Bind(1, std::to_string(userId));
	return query.Step() ? std::optional(ReadMatch(query)) : std::nullopt;
}

std::vector<Match> MatchStore::Favorites(std::int64_t userId, int offset, int limit) const
{
	AssertIsValid(offset >= 0 && limit > 0 && limit <= 10, "Некорректная страница избранного");
	Statement query(
		m_database,
		SelectMatches() +
			"JOIN review_decisions d ON d.match_id=m.id "
			"WHERE d.reviewer_id=? AND d.decision='like' ORDER BY m.rank LIMIT ? OFFSET ?");
	query.Bind(1, std::to_string(userId));
	query.Bind(2, limit);
	query.Bind(3, offset);
	std::vector<Match> matches;
	while (query.Step())
	{
		matches.push_back(ReadMatch(query));
	}
	return matches;
}

Progress MatchStore::GetProgress(std::int64_t userId) const
{
	Progress progress;
	progress.total = Count(m_database, "SELECT count(*) FROM matches");
	Statement query(m_database, "SELECT decision,count(*) FROM review_decisions "
								"WHERE reviewer_id=? GROUP BY decision");
	query.Bind(1, std::to_string(userId));
	while (query.Step())
	{
		auto& count = query.Text(0) == "like" ? progress.liked : progress.skipped;
		count += static_cast<int>(query.Integer(1));
	}
	return progress;
}

Draft MatchStore::GetDraft(const std::string& matchId, const std::string& code) const
{
	Statement query(m_database, "SELECT template_label,context_text,body_text,applicable "
								"FROM drafts WHERE match_id=? AND template_code=?");
	query.Bind(1, matchId);
	query.Bind(2, code);
	AssertIsValid(query.Step(), "Текст обращения не найден");
	return {query.Text(0), query.Text(1), query.Text(2), query.Integer(3) != 0};
}

std::string MatchStore::GetMode(std::int64_t userId, const Match& match) const
{
	Statement query(m_database, "SELECT mode FROM bot_modes WHERE user_id=? AND match_id=?");
	query.Bind(1, std::to_string(userId));
	query.Bind(2, match.id);
	return query.Step() ? query.Text(0) : match.suggestedMode;
}

void MatchStore::SetMode(std::int64_t userId, const std::string& matchId, const std::string& mode)
{
	AssertIsValid(mode == "buy_side" || mode == "sell_side", "Неизвестный сценарий обращения");
	Statement query(m_database, "INSERT INTO bot_modes VALUES(?,?,?) "
								"ON CONFLICT(user_id,match_id) DO UPDATE SET mode=excluded.mode");
	query.Bind(1, std::to_string(userId));
	query.Bind(2, matchId);
	query.Bind(3, mode);
	query.Step();
}

std::string MatchStore::GetDecision(std::int64_t userId, const std::string& matchId) const
{
	Statement query(m_database,
					"SELECT decision FROM review_decisions WHERE reviewer_id=? AND match_id=?");
	query.Bind(1, std::to_string(userId));
	query.Bind(2, matchId);
	return query.Step() ? query.Text(0) : "";
}

bool MatchStore::Decide(std::int64_t userId, const std::string& matchId,
						const std::string& decision, std::int64_t updateId, bool replaceLike)
{
	AssertIsValid(decision == "like" || decision == "dislike", "Неизвестное решение по паре");
	Transaction transaction(m_database);
	const auto previous = GetDecision(userId, matchId);
	if ((!replaceLike && !previous.empty()) || (replaceLike && previous != "like"))
	{
		return false;
	}
	Statement query(m_database,
					"INSERT OR IGNORE INTO bot_history "
					"(user_id,match_id,previous_decision,decision,update_id) VALUES(?,?,?,?,?)");
	query.Bind(1, std::to_string(userId));
	query.Bind(2, matchId);
	query.Bind(3, previous);
	query.Bind(4, decision);
	query.Bind(5, updateId);
	query.Step();
	if (m_database.GetChanges() == 0)
	{
		return false;
	}
	SaveDecision(m_database, userId, matchId, decision);
	transaction.Commit();
	return true;
}

std::optional<DecisionEvent> MatchStore::LastDecision(std::int64_t userId) const
{
	Statement query(m_database, "SELECT id,match_id FROM bot_history "
								"WHERE user_id=? AND undone=0 ORDER BY id DESC LIMIT 1");
	query.Bind(1, std::to_string(userId));
	return query.Step() ? std::optional(DecisionEvent{query.Integer(0), query.Text(1)})
						: std::nullopt;
}

std::optional<std::string> MatchStore::Undo(std::int64_t userId, std::int64_t eventId,
											std::int64_t updateId)
{
	Transaction transaction(m_database);
	Statement operation(m_database, "INSERT OR IGNORE INTO bot_undo_updates VALUES(?)");
	operation.Bind(1, updateId);
	operation.Step();
	if (m_database.GetChanges() == 0)
	{
		return std::nullopt;
	}
	const auto last = LastDecision(userId);
	if (!last || last->id != eventId)
	{
		return std::nullopt;
	}
	Statement previous(m_database, "SELECT previous_decision FROM bot_history WHERE id=?");
	previous.Bind(1, eventId);
	AssertIsValid(previous.Step(), "История решения не найдена");
	const auto value = previous.Text(0);
	if (value.empty())
	{
		Statement remove(m_database,
						 "DELETE FROM review_decisions WHERE reviewer_id=? AND match_id=?");
		remove.Bind(1, std::to_string(userId));
		remove.Bind(2, last->matchId);
		remove.Step();
	}
	else
	{
		SaveDecision(m_database, userId, last->matchId, value);
	}
	Statement update(m_database, "UPDATE bot_history SET undone=1 WHERE id=?");
	update.Bind(1, eventId);
	update.Step();
	transaction.Commit();
	return last->matchId;
}

std::int64_t MatchStore::GetOffset(const std::string& botId) const
{
	Statement query(m_database, "SELECT next_update_id FROM bot_offsets WHERE bot_id=?");
	query.Bind(1, botId);
	return query.Step() ? query.Integer(0) : 0;
}

void MatchStore::SetOffset(const std::string& botId, std::int64_t offset)
{
	Statement query(
		m_database,
		"INSERT INTO bot_offsets VALUES(?,?) ON CONFLICT(bot_id) "
		"DO UPDATE SET next_update_id=MAX(bot_offsets.next_update_id,excluded.next_update_id)");
	query.Bind(1, botId);
	query.Bind(2, offset);
	query.Step();
}
