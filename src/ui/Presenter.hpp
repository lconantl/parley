#pragma once

#include "model/Match.hpp"

namespace Presenter
{
std::string Card(const Match& match, const Progress& progress, const std::string& decision);
std::string Statistics(const Progress& progress);
std::string ModeLabel(const std::string& mode);
}
