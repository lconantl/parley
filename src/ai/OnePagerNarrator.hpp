#pragma once

#include "DueDiligenceNarrator.hpp"
#include "PolzaClient.hpp"
#include "common/output/CompanyAnonymizer.hpp"
#include "finance/CompanyAnalytics.hpp"

#include <memory>
#include <string>
#include <vector>

struct OnePagerNarrative
{
	std::string headline;
	std::vector<NarrativePoint> investmentCase;
	std::vector<NarrativePoint> keyRisks;
	std::string valuationTakeaway;
	std::vector<std::string> valueCreationActions;
	std::string nextStep;
};

class OnePagerNarrator
{
public:
	explicit OnePagerNarrator(std::shared_ptr<PolzaClient> client);

	OnePagerNarrative Compose(
		const CompanyAnalytics& analytics,
		const AnonymousIdentity& identity) const;

	static OnePagerNarrative BuildFallback(const CompanyAnalytics& analytics);

private:
	std::string BuildPrompt(
		const CompanyAnalytics& analytics,
		const AnonymousIdentity& identity) const;

	std::shared_ptr<PolzaClient> m_client;
};
