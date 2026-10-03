#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

class Config
{
  public:
	static Config Load(const std::filesystem::path& envPath);
	bool IsAllowed(std::int64_t userId) const noexcept;
	const std::string& GetToken() const noexcept;
	const std::filesystem::path& GetDatabasePath() const noexcept;
	const std::filesystem::path& GetSeedPath() const noexcept;

  private:
	std::string m_token;
	std::vector<std::int64_t> m_allowedUsers;
	std::filesystem::path m_databasePath;
	std::filesystem::path m_seedPath;
};
