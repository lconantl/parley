#pragma once

#include "domain/contact/ContactChannel.hpp"
#include "domain/contact/ContactSource.hpp"

#include <string>
#include <vector>

struct RawContact
{
	std::string firstName;
	std::string lastName;
	std::string role;
	std::string location;
	std::string notes;
	std::vector<ContactChannel> channels;
	ContactSource source = ContactSource::PhoneBook;
};
