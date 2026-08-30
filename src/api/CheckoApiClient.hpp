#pragma once

#include "CheckoApiTypes.hpp"
#include <memory>
#include <string>

class CheckoApiClient
{
public:
	explicit CheckoApiClient(std::string apiKey);
	~CheckoApiClient();

	CheckoResponse GetCompany(const std::string& identifier) const;
	CheckoResponse GetEntrepreneur(const std::string& identifier) const;
	CheckoResponse GetPerson(const std::string& identifier) const;

	CheckoResponse GetTimeline(
		const std::string& identifier,
		const std::unordered_map<std::string, std::string>& parameters) const;

	CheckoResponse Search(
		const std::unordered_map<std::string, std::string>& parameters) const;

	CheckoResponse GetFinances(
		const std::string& identifier,
		const std::unordered_map<std::string, std::string>& parameters) const;

	CheckoResponse GetContracts(
		const std::string& identifier,
		const std::unordered_map<std::string, std::string>& parameters) const;

	CheckoResponse GetInspections(
		const std::string& identifier,
		const std::unordered_map<std::string, std::string>& parameters) const;

	CheckoResponse GetEnforcements(
		const std::string& identifier,
		const std::unordered_map<std::string, std::string>& parameters) const;

	CheckoResponse GetLegalCases(
		const std::string& identifier,
		const std::unordered_map<std::string, std::string>& parameters) const;

	CheckoResponse GetFedresurs(
		const std::string& identifier,
		const std::unordered_map<std::string, std::string>& parameters) const;

	CheckoResponse GetBankruptcyMessages(
		const std::string& identifier,
		const std::unordered_map<std::string, std::string>& parameters) const;

	CheckoResponse GetBank(
		const std::unordered_map<std::string, std::string>& parameters) const;

	CheckoApiStatistics GetStatistics() const;

private:
	class Impl;

	std::unique_ptr<Impl> m_impl;
};