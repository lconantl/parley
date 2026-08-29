#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace EnvLoader
{
inline constexpr std::string_view FILE_NAME = ".env";
inline constexpr std::string_view MOCK_FILE_NAME = ".env.mock";

using EnvData = std::unordered_map<std::string, std::string>;

class LoadError : public std::runtime_error
{
public:
	using std::runtime_error::runtime_error;
};

EnvData Load(const std::filesystem::path& path = FILE_NAME);
void PrintMap(const EnvData& data);
} // namespace EnvLoader