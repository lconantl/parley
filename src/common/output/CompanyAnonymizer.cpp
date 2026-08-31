#include "CompanyAnonymizer.hpp"
#include "finance/Metric.hpp"
#include <algorithm>

namespace
{
constexpr auto Subject = "Целевая компания";
constexpr auto UnknownIndustry = "Отрасль не определена";
constexpr auto UnknownRegion = "Регион не определен";

constexpr double SmallScaleLimit = 800000000.0;
constexpr double MediumScaleLimit = 8000000000.0;

std::string ExtractRegion(const std::string& address)
{
	const std::size_t separator = address.find(',');
	if (separator == std::string::npos)
	{
		return address.empty() ? UnknownRegion : address;
	}

	std::string tail = address.substr(separator + 1);
	const std::size_t next = tail.find(',');
	if (next != std::string::npos)
	{
		tail = tail.substr(0, next);
	}

	const std::size_t begin = tail.find_first_not_of(' ');

	return begin == std::string::npos ? UnknownRegion : tail.substr(begin);
}

std::string DescribeScale(const MetricValue& revenue)
{
	if (!Metric::IsKnown(revenue))
	{
		return "Масштаб не определен";
	}

	if (revenue.value < SmallScaleLimit)
	{
		return "Малый бизнес";
	}

	if (revenue.value < MediumScaleLimit)
	{
		return "Средний бизнес";
	}

	return "Крупный бизнес";
}

std::string MakeCompetitorLabel(const std::size_t index)
{
	const char letter = static_cast<char>('A' + index % 26);

	return std::string("Конкурент ") + letter;
}

void ReplaceAll(std::string& text, const std::string& what, const std::string& with)
{
	if (what.empty())
	{
		return;
	}

	std::size_t position = text.find(what);
	while (position != std::string::npos)
	{
		text.replace(position, what.size(), with);
		position = text.find(what, position + with.size());
	}
}

std::string StripQuotes(const std::string& name)
{
	std::string stripped;

	for (std::size_t index = 0; index < name.size(); ++index)
	{
		const unsigned char symbol = static_cast<unsigned char>(name[index]);
		if (symbol == '"' || symbol == '\'')
		{
			continue;
		}

		stripped += name[index];
	}

	return stripped;
}
} // namespace

std::string CompanyAnonymizer::SubjectLabel()
{
	return Subject;
}

AnonymousIdentity CompanyAnonymizer::Describe(const CompanyAnalytics& analytics)
{
	AnonymousIdentity identity;
	identity.subject = Subject;
	identity.industry = analytics.activity.empty() ? UnknownIndustry : analytics.activity;
	identity.region = ExtractRegion(analytics.region);
	identity.scale = DescribeScale(analytics.revenue.revenue);

	const int firstYear = analytics.series.empty()
		? analytics.year
		: analytics.series.back().year;

	identity.period = std::to_string(firstYear) + "–" + std::to_string(analytics.year);

	return identity;
}

std::vector<Competitor> CompanyAnonymizer::MaskCompetitors(
	const std::vector<Competitor>& competitors)
{
	std::vector<Competitor> masked;
	masked.reserve(competitors.size());

	for (std::size_t index = 0; index < competitors.size(); ++index)
	{
		Competitor entry = competitors[index];
		entry.name = MakeCompetitorLabel(index);
		entry.inn.clear();

		masked.push_back(std::move(entry));
	}

	return masked;
}

std::string CompanyAnonymizer::MaskText(
	const std::string& text,
	const CompanyAnalytics& analytics)
{
	std::string masked = text;

	ReplaceAll(masked, analytics.name, Subject);
	ReplaceAll(masked, StripQuotes(analytics.name), Subject);
	ReplaceAll(masked, analytics.identifier, Subject);

	for (const auto& party : analytics.relatedParties)
	{
		if (party.name.size() > 3)
		{
			ReplaceAll(masked, party.name, "связанное лицо");
		}
	}

	return masked;
}