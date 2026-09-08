#include "PriceLabeler.hpp"

#include <random>

namespace
{
constexpr int ReportMin = 50;
constexpr int ReportMax = 90;
constexpr int PresentationMin = 90;
constexpr int PresentationMax = 120;
constexpr int OnePagerMin = 85;
constexpr int OnePagerMax = 132;

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
