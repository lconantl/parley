#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct PptxImage
{
	std::filesystem::path path;
	double xInches = 0.0;
	double yInches = 0.0;
	double widthInches = 0.0;
	double heightInches = 0.0;
};

struct PptxSlide
{
	std::string title;
	std::vector<std::string> bullets;
	std::optional<std::string> backgroundColor;
	std::vector<PptxImage> images;
};
