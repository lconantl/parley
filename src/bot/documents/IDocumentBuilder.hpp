#pragma once

#include "finance/CompanyAnalytics.hpp"

#include <filesystem>
#include <string>

struct DocumentBuildOptions
{
	bool anonymize = false;
	bool showSourceNotes = false;
	std::string author = "Investment Analysis";
};

struct DocumentBuildResult
{
	std::filesystem::path path;
	std::string caption;
	std::string mimeType;
};

class IDocumentBuilder
{
public:
	virtual ~IDocumentBuilder() = default;

	virtual std::string GetLabel() const = 0;
	virtual DocumentBuildResult Build(
		const CompanyAnalytics& analytics,
		const DocumentBuildOptions& options) const = 0;
};
