#pragma once

#include "application/importer/RawContact.hpp"
#include "domain/contact/Contact.hpp"

#include <string>

class ContactNormalizer
{
public:
	static Contact Normalize(const RawContact& raw);

	static std::string NormalizePhone(const std::string& rawPhone);
	static std::string NormalizeTelegramUsername(const std::string& rawUsername);

private:
	static std::string BuildCanonicalName(const std::string& firstName, const std::string& lastName);
};
