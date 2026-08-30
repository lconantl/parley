#pragma once

#include "CompanyProfile.hpp"
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>

class Company
{
public:
	explicit Company(std::string id);

	bool HasData(const std::string& method) const;
	void SetData(const std::string& method, nlohmann::json data);
	const nlohmann::json& GetData(const std::string& method) const;

	bool HasProfile() const;
	void SetProfile(CompanyProfile profile);
	CompanyProfile GetProfile() const;

	const std::string& GetIdentifier() const noexcept;
	std::size_t GetMethodCount() const noexcept;
	void PrintJson() const;

private:
	std::string m_id;
	std::unordered_map<std::string, nlohmann::json> m_data;
	std::optional<CompanyProfile> m_profile;
	mutable std::mutex m_mutex;
};