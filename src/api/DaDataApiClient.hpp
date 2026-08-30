#pragma once

#include "model/ DaDataApiTypes.hpp"
#include <memory>
#include <string>

class DaDataApiClient
{
public:
	DaDataApiClient(std::string apiKey, std::string secretKey);
	~DaDataApiClient();

	DaDataResponse FindParty(const std::string& identifier) const;
	DaDataResponse FindBranches(const std::string& identifier, std::size_t count) const;
	DaDataResponse FindAffiliated(const std::string& identifier, std::size_t count) const;
	DaDataResponse FindBrand(const std::string& identifier) const;
	DaDataResponse GetBalance() const;

	DaDataApiStatistics GetStatistics() const;

private:
	class Impl;

	std::unique_ptr<Impl> m_impl;
};