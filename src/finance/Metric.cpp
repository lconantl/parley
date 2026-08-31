#include "Metric.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
constexpr double MinConfidence = 0.0;
constexpr double MaxConfidence = 1.0;

double ClampConfidence(const double confidence)
{
	return std::clamp(confidence, MinConfidence, MaxConfidence);
}

void AssertIsMetricKnown(const bool known)
{
	if (!known)
	{
		throw std::out_of_range("Значение показателя не определено");
	}
}
} // namespace

MetricValue Metric::Reported(const double value)
{
	return {value, MetricOrigin::Reported, MaxConfidence, {}};
}

MetricValue Metric::Computed(const double value, const double confidence)
{
	return {value, MetricOrigin::Computed, ClampConfidence(confidence), {}};
}

MetricValue Metric::Estimated(const double value, const double confidence, std::string comment)
{
	return {value, MetricOrigin::Estimated, ClampConfidence(confidence), std::move(comment)};
}

MetricValue Metric::Unavailable(std::string comment)
{
	return {0.0, MetricOrigin::Unavailable, MinConfidence, std::move(comment)};
}

MetricValue Metric::NotApplicable(std::string comment)
{
	return {0.0, MetricOrigin::NotApplicable, MinConfidence, std::move(comment)};
}

bool Metric::IsKnown(const MetricValue& metric) noexcept
{
	return metric.origin == MetricOrigin::Reported
		|| metric.origin == MetricOrigin::Computed
		|| metric.origin == MetricOrigin::Estimated;
}

bool Metric::IsMissing(const MetricValue& metric) noexcept
{
	return metric.origin == MetricOrigin::Unavailable;
}

std::optional<double> Metric::Number(const MetricValue& metric)
{
	if (!IsKnown(metric))
	{
		return std::nullopt;
	}

	return metric.value;
}

double Metric::CombineConfidence(const double first, const double second)
{
	return ClampConfidence(std::min(first, second));
}

double Metric::CombineConfidence(const double first, const double second, const double third)
{
	return CombineConfidence(CombineConfidence(first, second), third);
}

std::string Metric::DescribeOrigin(const MetricOrigin origin)
{
	switch (origin)
	{
	case MetricOrigin::Reported:
		return "из отчетности";
	case MetricOrigin::Computed:
		return "расчет";
	case MetricOrigin::Estimated:
		return "оценка";
	case MetricOrigin::NotApplicable:
		return "неприменимо";
	case MetricOrigin::Unavailable:
		return "нет данных";
	}

	AssertIsMetricKnown(false);

	return {};
}

std::string Metric::DescribeUnit(const MetricUnit unit)
{
	switch (unit)
	{
	case MetricUnit::Money:
		return "рубли";
	case MetricUnit::Percent:
		return "проценты";
	case MetricUnit::Ratio:
		return "коэффициент";
	case MetricUnit::Days:
		return "дни";
	case MetricUnit::Count:
		return "штуки";
	case MetricUnit::Text:
		return "текст";
	}

	return "рубли";
}