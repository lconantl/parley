#pragma once

#include "domain/contact/Contact.hpp"
#include "domain/contact/ContactChannel.hpp"
#include "domain/search/SearchCriteria.hpp"
#include "domain/search/SearchResult.hpp"

#include <cstddef>
#include <optional>
#include <vector>

class IContactRepository
{
public:
	virtual ~IContactRepository() = default;

	virtual std::optional<Contact> FindByChannel(const ContactChannel& channel) const = 0;
	virtual Contact Upsert(const Contact& contact) = 0;
	virtual std::vector<SearchResult> Search(const SearchCriteria& criteria, std::size_t limit) const = 0;
};
