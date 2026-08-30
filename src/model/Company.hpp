#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>

class Company
{
public:
	explicit Company(std::string id);

	bool HasData(const std::string& method) const;
	void SetData(const std::string& method, nlohmann::json data);
	const nlohmann::json& GetData(const std::string& method) const;

	const std::string& GetIdentifier() const noexcept;
	std::size_t GetMethodCount() const noexcept;

private:
	std::string m_id;
	std::unordered_map<std::string, nlohmann::json> m_data;
};