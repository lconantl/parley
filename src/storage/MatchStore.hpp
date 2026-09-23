#pragma once

#include "Database.hpp"
#include "model/Match.hpp"
#include <optional>

class MatchStore
{
  public:
	explicit MatchStore(const std::filesystem::path& path);
	void Validate() const;
	void Migrate();
	std::optional<Match> FindMatch(const std::string& matchId) const;
	std::optional<Match> Next(std::int64_t userId) const;
	std::vector<Match> Favorites(std::int64_t userId, int offset, int limit = 5) const;
	Progress GetProgress(std::int64_t userId) const;
	Draft GetDraft(const std::string& matchId, const std::string& code) const;
	std::string GetMode(std::int64_t userId, const Match& match) const;
	void SetMode(std::int64_t userId, const std::string& matchId, const std::string& mode);
	bool Decide(std::int64_t userId, const std::string& matchId, const std::string& decision,
				std::int64_t updateId, bool replaceLike = false);
	std::optional<DecisionEvent> LastDecision(std::int64_t userId) const;
	std::optional<std::string> Undo(std::int64_t userId, std::int64_t eventId,
									std::int64_t updateId);
	std::string GetDecision(std::int64_t userId, const std::string& matchId) const;
	std::int64_t GetOffset(const std::string& botId) const;
	void SetOffset(const std::string& botId, std::int64_t offset);

  private:
	Database m_database;
};
