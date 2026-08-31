#pragma once

#include "CompanyAnonymizer.hpp"
#include "ai/DueDiligenceNarrator.hpp"
#include "common/output/pdf/model/Deck.hpp"
#include "finance/CompanyAnalytics.hpp"
#include "finance/MetricFormatter.hpp"

#include <string>
#include <vector>

struct DueDiligenceOptions
{
	bool anonymize = true;
	bool showSourceNotes = true;
	std::string author = "Investment Analysis";
};

class DueDiligenceDeckBuilder
{
public:
	DueDiligenceDeckBuilder(DueDiligenceOptions options, MetricFormatOptions formatOptions);

	Deck Build(const CompanyAnalytics& analytics, const DueDiligenceNarrative& narrative) const;

private:
	Slide BuildTitle(const AnonymousIdentity& identity) const;
	Slide BuildExecutiveSummary(const DueDiligenceNarrative& narrative) const;
	Slide BuildProfile(const CompanyAnalytics& analytics, const AnonymousIdentity& identity, const DueDiligenceNarrative& narrative) const;
	Slide BuildFinancialResults(const CompanyAnalytics& analytics,
		const DueDiligenceNarrative& narrative) const;
	Slide BuildRevenueChart(const CompanyAnalytics& analytics,
		const DueDiligenceNarrative& narrative) const;
	Slide BuildMargins(const CompanyAnalytics& analytics,
		const DueDiligenceNarrative& narrative) const;
	Slide BuildCashFlow(const CompanyAnalytics& analytics,
		const DueDiligenceNarrative& narrative) const;
	Slide BuildBalance(const CompanyAnalytics& analytics,
		const DueDiligenceNarrative& narrative) const;
	Slide BuildQuality(const CompanyAnalytics& analytics,
		const DueDiligenceNarrative& narrative) const;
	Slide BuildReturns(const CompanyAnalytics& analytics,
		const DueDiligenceNarrative& narrative) const;
	Slide BuildMarket(const CompanyAnalytics& analytics,
		const DueDiligenceNarrative& narrative) const;
	Slide BuildCompetitors(const CompanyAnalytics& analytics,
		const DueDiligenceNarrative& narrative) const;
	Slide BuildRisks(const DueDiligenceNarrative& narrative) const;
	Slide BuildOpportunities(const DueDiligenceNarrative& narrative) const;
	Slide BuildValuation(const CompanyAnalytics& analytics,
		const DueDiligenceNarrative& narrative) const;
	Slide BuildOpenQuestions(const CompanyAnalytics& analytics,
		const DueDiligenceNarrative& narrative) const;
	Slide BuildConclusion(const DueDiligenceNarrative& narrative) const;

	std::string Money(const MetricValue& metric) const;
	std::string Percent(const MetricValue& metric) const;
	std::string Ratio(const MetricValue& metric) const;
	std::string SourceNote(const std::string& sources) const;

	DueDiligenceOptions m_options;
	MetricFormatter m_formatter;
};