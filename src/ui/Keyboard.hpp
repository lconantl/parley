#pragma once

#include "model/Match.hpp"
#include <nlohmann/json.hpp>
#include <optional>

namespace Keyboard
{
nlohmann::json Main();
nlohmann::json Continue();
nlohmann::json Card(const Match& match, const std::string& decision,
					const std::optional<DecisionEvent>& last);
nlohmann::json Draft(const Match& match, const std::string& mode);
nlohmann::json Modes(const Match& match);
nlohmann::json Back(const Match& match);
nlohmann::json Favorites(const std::vector<Match>& matches, int offset, int total);
nlohmann::json Empty(const std::optional<DecisionEvent>& last);
}
