#pragma once

#include "application/importer/RawContact.hpp"

#include <string>

enum class ColumnKind
{
	FirstName,
	LastName,
	FullName,
	Role,
	Location,
	Phone,
	Email,
	Telegram,
	Contact,
	Notes,
	Unknown
};

namespace ContactRowMapper
{
ColumnKind ClassifyHeader(const std::string& header);
void ApplyColumnValue(ColumnKind kind, const std::string& value, RawContact& contact);
} // namespace ContactRowMapper
