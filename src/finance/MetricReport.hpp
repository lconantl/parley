#pragma once

#include "CompanyAnalytics.hpp"
#include "MetricValue.hpp"
#include <string>
#include <vector>

struct MetricRow
{
	std::string id;
	std::string title;
	MetricUnit unit = MetricUnit::Money;
	MetricGroup group = MetricGroup::Revenue;
	MetricValue value;
};

class MetricReport
{
public:
	explicit MetricReport(const CompanyAnalytics& analytics);

	const std::vector<MetricRow>& GetRows() const noexcept;
	std::vector<MetricRow> GetGroup(MetricGroup group) const;

	static std::vector<MetricGroup> ListGroups();
	static std::string DescribeGroup(MetricGroup group);

private:
	std::vector<MetricRow> m_rows;
};
