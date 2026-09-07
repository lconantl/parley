#pragma once

#include "MetricValue.hpp"
#include <string>

struct MetricFormatOptions
{
	bool showOrigin = true;
	bool showConfidence = true;
	bool showComment = false;
	bool showMissing = true;
	bool showNotApplicable = true;
	bool compactMoney = false;
	bool moneyInMillions = false;
	std::string missingText = "нет данных";
};

class MetricFormatter
{
public:
	explicit MetricFormatter(MetricFormatOptions options);

	bool ShouldShow(const MetricValue& metric) const;
	bool ShowsOrigin() const noexcept;
	bool ShowsConfidence() const noexcept;

	std::string FormatValue(const MetricValue& metric, MetricUnit unit) const;
	std::string FormatOrigin(const MetricValue& metric) const;
	std::string FormatConfidence(const MetricValue& metric) const;
	std::string FormatFull(const MetricValue& metric, MetricUnit unit) const;

	static std::string FormatMoney(double value, bool compact);
	static std::string FormatMoneyInMillions(double value);
	static std::string FormatNumber(double value, int decimals);

private:
	MetricFormatOptions m_options;
};
