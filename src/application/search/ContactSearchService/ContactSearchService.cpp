#include "ContactSearchService.hpp"

#include <stdexcept>
#include <utility>

namespace
{
void AssertIsQueryNotEmpty(const std::string& queryText)
{
	if (queryText.empty())
	{
		throw std::invalid_argument("Поисковый запрос не может быть пустым");
	}
}
} // namespace

ContactSearchService::ContactSearchService(
	std::shared_ptr<IContactRepository> repository,
	std::shared_ptr<IQueryInterpreter> interpreter)
	: m_repository(std::move(repository))
	, m_interpreter(std::move(interpreter))
{
}

std::vector<SearchResult> ContactSearchService::Search(const std::string& queryText) const
{
	AssertIsQueryNotEmpty(queryText);

	const SearchCriteria criteria = m_interpreter->Interpret(queryText);

	return m_repository->Search(criteria, MaxResults);
}
