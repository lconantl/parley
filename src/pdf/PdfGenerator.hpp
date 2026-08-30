#pragma once

#include "layout/Deck.hpp"
#include "theme/Theme.hpp"
#include <filesystem>

class PdfGenerator
{
public:
	static void Generate(const Deck& deck, const Theme& theme, const std::filesystem::path& outputPath);
};