#pragma once

#include "common/output/pdf/model/Deck.hpp"
#include "common/output/pdf/theme/Theme.hpp"
#include <filesystem>

class PdfGenerator
{
public:
	static void Generate(const Deck& deck, const Theme& theme, const std::filesystem::path& outputPath);
};