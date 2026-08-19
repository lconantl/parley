#pragma once

#include <filesystem>
#include <string>
#include <vector>

class Config
{
public:
	static Config Load(const std::filesystem::path& filePath = "../.env");

	const std::string& GetBotToken() const;
	const std::vector<std::string>& GetAllowedUsers() const;
	const std::string& GetBfoBaseUrl() const;
	const std::string& GetBfoSearchPath() const;
	long GetBfoPageSize() const;
	long GetBfoConnectTimeoutSec() const;
	long GetBfoReadTimeoutSec() const;
	long GetBfoWriteTimeoutSec() const;
	const std::string& GetBfoUserAgent() const;
	const std::string& GetPolzaBaseUrl() const;
	const std::string& GetPolzaApiKey() const;
	const std::string& GetPolzaModel() const;

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
		std::string bfoUserAgent,
		std::string polzaBaseUrl,
		std::string polzaApiKey,
		std::string polzaModel);

	std::string m_botToken;
	std::vector<std::string> m_allowedUsers;

	std::string m_bfoBaseUrl;
	std::string m_bfoSearchPath;

	long m_bfoPageSize;

	long m_bfoConnectTimeoutSec;
	long m_bfoReadTimeoutSec;
	long m_bfoWriteTimeoutSec;

	std::string m_bfoUserAgent;

	std::string m_polzaBaseUrl;
	std::string m_polzaApiKey;
	std::string m_polzaModel;
};