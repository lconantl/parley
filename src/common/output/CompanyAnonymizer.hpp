#pragma once

#include "finance/CompanyAnalytics.hpp"
#include <string>
#include <vector>

struct AnonymousIdentity
{
	std::string subject;
	std::string industry;
	std::string region;
	std::string scale;
	std::string period;
};

class CompanyAnonymizer
{
public:
	static AnonymousIdentity Describe(const CompanyAnalytics& analytics);
	static std::vector<Competitor> MaskCompetitors(const std::vector<Competitor>& competitors);
	static std::string MaskText(const std::string& text, const CompanyAnalytics& analytics);

	static std::string SubjectLabel();
};