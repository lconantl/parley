#include "PriceLabeler.hpp"

#include <random>

namespace
{
constexpr int ReportMin = 90;
constexpr int ReportMax = 120;
constexpr int PresentationMin = 120;
constexpr int PresentationMax = 250;
constexpr int OnePagerMin = 120;
constexpr int OnePagerMax = 200;

std::mt19937& RandomEngine()
{
	thread_local std::mt19937 engine(std::random_device{}());

	return engine;
}

int RollBetween(const int minimum, const int maximum)
{
	std::uniform_int_distribution<int> distribution(minimum, maximum);

	return distribution(RandomEngine());
}
} // namespace

PriceQuote PriceLabeler::RollPrices()
{
	return {
		RollBetween(ReportMin, ReportMax),
		RollBetween(PresentationMin, PresentationMax),
		RollBetween(OnePagerMin, OnePagerMax)};
}

int PriceLabeler::AllInclusivePrice(const PriceQuote& prices)
{
	return prices.report + prices.presentation + prices.onePager;
}
