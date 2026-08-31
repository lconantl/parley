#pragma once

#include "PolzaClient.hpp"
#include "common/output/CompanyAnonymizer.hpp"
#include "finance/CompanyAnalytics.hpp"
#include <memory>
#include <string>
#include <vector>

struct NarrativePoint
{
	std::string title;
	std::string body;
};

struct DueDiligenceNarrative
{
	std::vector<std::string> executiveSummary;
	std::string investmentVerdict;
	std::string businessProfile;
	std::string positioning;
	std::string marketCommentary;
	std::string valuationCommentary;
	std::vector<NarrativePoint> risks;
	std::vector<NarrativePoint> opportunities;
	std::vector<std::string> openQuestions;
	std::vector<std::string> nextSteps;
	std::string conclusion;

	std::string revenueTakeaway;
	std::string marginTakeaway;
	std::string cashFlowTakeaway;
	std::string balanceTakeaway;
	std::string qualityTakeaway;
	std::string returnsTakeaway;
	std::string competitionTakeaway;
};

class DueDiligenceNarrator
{
public:
	explicit DueDiligenceNarrator(std::shared_ptr<PolzaClient> client);

	DueDiligenceNarrative Compose(
		const CompanyAnalytics& analytics,
		const AnonymousIdentity& identity) const;

	static DueDiligenceNarrative BuildFallback(const CompanyAnalytics& analytics);

private:
	std::string BuildPrompt(
		const CompanyAnalytics& analytics,
		const AnonymousIdentity& identity) const;

	std::shared_ptr<PolzaClient> m_client;
};