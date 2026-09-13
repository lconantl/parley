#pragma once

#include "application/interpreter/IQueryInterpreter.hpp"
#include "application/repository/IContactRepository.hpp"
#include "domain/search/SearchResult.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class ContactSearchService
{
public:
	ContactSearchService(
		std::shared_ptr<IContactRepository> repository,
		std::shared_ptr<IQueryInterpreter> interpreter);

	std::vector<SearchResult> Search(const std::string& queryText) const;

private:
	static constexpr std::size_t MaxResults = 5;

	std::shared_ptr<IContactRepository> m_repository;
	std::shared_ptr<IQueryInterpreter> m_interpreter;
};
