#pragma once

#include "MetricValue.hpp"
#include <optional>
#include <string>

class Metric
{
public:
	static MetricValue Reported(double value);
	static MetricValue Computed(double value, double confidence);
	static MetricValue Estimated(double value, double confidence, std::string comment);
	static MetricValue Unavailable(std::string comment);
	static MetricValue NotApplicable(std::string comment);

	static bool IsKnown(const MetricValue& metric) noexcept;
	static bool IsMissing(const MetricValue& metric) noexcept;
	static std::optional<double> Number(const MetricValue& metric);

	static double CombineConfidence(double first, double second);
	static double CombineConfidence(double first, double second, double third);

	static std::string DescribeOrigin(MetricOrigin origin);
	static std::string DescribeUnit(MetricUnit unit);
};