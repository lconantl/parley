#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

class EnvLoader
{
public:
	using EnvData = std::unordered_map<std::string, std::string>;

	static EnvData Load(const std::filesystem::path& filePath);
};