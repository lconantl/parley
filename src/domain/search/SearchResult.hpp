#pragma once

#include "domain/contact/Contact.hpp"

struct SearchResult
{
	Contact contact;
	double matchScore = 0.0;
};
