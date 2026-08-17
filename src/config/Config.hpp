#pragma once

#include <filesystem>
#include <string>
#include <vector>

class Config
{
public:
	static Config Load(
		const std::filesystem::path& filePath = "../.env");

	[[nodiscard]]
	const std::string& GetBotToken() const;

	[[nodiscard]]
	const std::vector<std::string>& GetAllowedUsers() const;

	[[nodiscard]]
	const std::string& GetBfoBaseUrl() const;

	[[nodiscard]]
	const std::string& GetBfoSearchPath() const;

	[[nodiscard]]
	long GetBfoPageSize() const;

	[[nodiscard]]
	long GetBfoConnectTimeoutSec() const;

	[[nodiscard]]
	long GetBfoReadTimeoutSec() const;

	[[nodiscard]]
	long GetBfoWriteTimeoutSec() const;

	[[nodiscard]]
	const std::string& GetBfoUserAgent() const;

private:
	Config(
		std::string botToken,
		std::vector<std::string> allowedUsers,
		std::string bfoBaseUrl,
		std::string bfoSearchPath,
		long bfoPageSize,
		long bfoConnectTimeoutSec,
		long bfoReadTimeoutSec,
		long bfoWriteTimeoutSec,
		std::string bfoUserAgent);

	std::string m_botToken;
	std::vector<std::string> m_allowedUsers;

	std::string m_bfoBaseUrl;
	std::string m_bfoSearchPath;

	long m_bfoPageSize;

	long m_bfoConnectTimeoutSec;
	long m_bfoReadTimeoutSec;
	long m_bfoWriteTimeoutSec;

	std::string m_bfoUserAgent;
};