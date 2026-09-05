#include "OnePagerNarrator.hpp"

#include "finance/Metric.hpp"
#include "finance/MetricFormatter.hpp"

#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{
constexpr std::size_t MaxInvestmentCasePoints = 3;
constexpr std::size_t MaxRiskPoints = 3;
constexpr std::size_t MaxValueCreationActions = 3;

void AssertIsClientValid(const std::shared_ptr<PolzaClient>& client)
{
	if (client == nullptr)
	{
		throw std::invalid_argument("Клиент нейросети не может быть пустым");
	}
}

std::string SystemPrompt()
{
	return "Ты пишешь тексты для одностраничного инвестиционного тизера (sell-side one-pager) "
		   "для M&A-сделки. Стиль — сдержанный, но энергичный, как у корпоративных финансов: "
		   "короткие фразы, конкретика, ноль воды и ноль рекламных прилагательных без опоры на "
		   "цифры.\n"
		   "Жесткие правила:\n"
		   "1. Объект называется только «Целевая компания». Настоящее название, ИНН, имена "
		   "собственников и руководителей упоминать запрещено.\n"
		   "2. Не упоминай источники данных, методику и то, кем подготовлен текст.\n"
		   "3. Не пиши о вероятностях, уверенности и приблизительности — утверждай прямо.\n"
		   "4. Никогда не изобретай цифры: используй только то, что дано в исходных данных, "
		   "если нужной цифры нет — пиши без нее.\n"
		   "5. В value_creation_actions не должно быть ни одного числа, процента или суммы — "
		   "только качественные шаги и эффекты.\n"
		   "6. Никогда не используй символ рубля (₽), всегда пиши «руб.».";
}

nlohmann::json TextField(const char* description)
{
	return {{"type", "string"}, {"description", description}};
}

nlohmann::json StringArray(const char* description)
{
	return {{"type", "array"}, {"items", {{"type", "string"}}}, {"description", description}};
}

nlohmann::json PointArray(const char* description)
{
	return {{"type", "array"},
		{"items",
			{{"type", "object"},
				{"properties",
					{{"title", TextField("краткая формулировка, до шести слов")},
						{"body", TextField("одно-два предложения с опорой на цифры")}}},
				{"required", nlohmann::json::array({"title", "body"})},
				{"additionalProperties", false}}},
		{"description", description}};
}

nlohmann::json BuildSchema()
{
	nlohmann::json properties = {
		{"headline", TextField("2-3 строки: позиционирование, масштаб и главное инвестиционное преимущество")},
		{"investment_case", PointArray("ровно три тезиса: Масштаб, Экономика, Создание стоимости")},
		{"key_risks", PointArray("2-3 ключевых риска, каждый до двух строк")},
		{"valuation_takeaway", TextField("одна фраза-вывод по оценке стоимости, может быть пустой строкой")},
		{"value_creation_actions",
			StringArray("2-3 качественных шага создания стоимости, без единой цифры/процента/суммы")},
		{"next_step", TextField("одна строка: следующий шаг сделки")}};

	nlohmann::json required = nlohmann::json::array();
	for (const auto& item : properties.items())
	{
		required.push_back(item.key());
	}

	return {{"type", "object"},
		{"properties", properties},
		{"required", required},
		{"additionalProperties", false}};
}

std::string ReadText(const nlohmann::json& node, const char* key)
{
	if (!node.contains(key) || !node.at(key).is_string())
	{
		return {};
	}

	return node.at(key).get<std::string>();
}

std::vector<std::string> ReadStrings(const nlohmann::json& node, const char* key)
{
	std::vector<std::string> values;

	if (!node.contains(key) || !node.at(key).is_array())
	{
		return values;
	}

	for (const auto& item : node.at(key))
	{
		if (item.is_string() && !item.get<std::string>().empty())
		{
			values.push_back(item.get<std::string>());
		}
	}

	return values;
}

std::vector<NarrativePoint> ReadPoints(const nlohmann::json& node, const char* key)
{
	std::vector<NarrativePoint> points;

	if (!node.contains(key) || !node.at(key).is_array())
	{
		return points;
	}

	for (const auto& item : node.at(key))
	{
		const std::string title = ReadText(item, "title");
		if (title.empty())
		{
			continue;
		}

		points.push_back({title, ReadText(item, "body")});
	}

	return points;
}

void CapTo(std::vector<NarrativePoint>& points, const std::size_t limit)
{
	if (points.size() > limit)
	{
		points.resize(limit);
	}
}

void CapTo(std::vector<std::string>& values, const std::size_t limit)
{
	if (values.size() > limit)
	{
		values.resize(limit);
	}
}

std::string Money(const MetricValue& metric)
{
	if (!Metric::IsKnown(metric))
	{
		return {};
	}

	return MetricFormatter::FormatMoney(metric.value, /*compact=*/true);
}

std::string Percent(const MetricValue& metric)
{
	if (!Metric::IsKnown(metric))
	{
		return {};
	}

	return MetricFormatter::FormatNumber(metric.value, 1) + " %";
}

std::string Ratio(const MetricValue& metric)
{
	if (!Metric::IsKnown(metric))
	{
		return {};
	}

	return MetricFormatter::FormatNumber(metric.value, 2) + "x";
}

void AppendKnownLine(std::ostringstream& stream, const std::string& label, const std::string& formatted)
{
	if (formatted.empty())
	{
		return;
	}

	stream << "- " << label << ": " << formatted << "\n";
}
} // namespace

OnePagerNarrator::OnePagerNarrator(std::shared_ptr<PolzaClient> client)
	: m_client(std::move(client))
{
	AssertIsClientValid(m_client);
}

std::string OnePagerNarrator::BuildPrompt(
	const CompanyAnalytics& analytics,
	const AnonymousIdentity& identity) const
{
	std::ostringstream stream;

	stream << "Объект: " << identity.subject << "\n"
		   << "Отрасль: " << identity.industry << "\n"
		   << "Регион: " << identity.region << "\n"
		   << "Масштаб: " << identity.scale << "\n"
		   << "Период: " << identity.period << "\n\n"
		   << "Ключевые цифры (только контекст, не изобретай новые):\n";

	AppendKnownLine(stream, "Выручка", Money(analytics.revenue.revenue));
	AppendKnownLine(stream, "Рост выручки", Percent(analytics.revenue.revenueGrowth));
	AppendKnownLine(stream, "EBITDA", Money(analytics.profit.ebitda));
	AppendKnownLine(stream, "Маржа EBITDA", Percent(analytics.margins.ebitdaMargin));
	AppendKnownLine(stream, "Свободный денежный поток", Money(analytics.cashFlow.freeCashFlow));
	AppendKnownLine(stream, "Чистый долг / EBITDA", Ratio(analytics.debt.netDebtToEbitda));
	AppendKnownLine(stream, "Enterprise Value", Money(analytics.valuation.enterpriseValue));
	AppendKnownLine(stream, "EV / EBITDA", Ratio(analytics.valuation.enterpriseToEbitda));
	AppendKnownLine(stream, "Equity Value (100% доля)", Money(analytics.valuation.marketCapitalization));

	if (Metric::IsKnown(analytics.customers.count))
	{
		AppendKnownLine(stream, "Клиентская база", MetricFormatter::FormatNumber(analytics.customers.count.value, 0));
	}

	if (!analytics.market.definition.empty())
	{
		stream << "- Рынок: " << analytics.market.definition;
		const std::string size = Money(analytics.market.size);
		const std::string growth = Percent(analytics.market.growth);
		if (!size.empty())
		{
			stream << ", размер " << size;
		}
		if (!growth.empty())
		{
			stream << ", рост " << growth;
		}
		stream << "\n";
	}

	std::size_t segmentsShown = 0;
	for (const auto& segment : analytics.segments)
	{
		if (segmentsShown >= 2 || !Metric::IsKnown(segment.revenue))
		{
			continue;
		}

		stream << "- Сегмент «" << segment.name << "»: " << Money(segment.revenue) << "\n";
		++segmentsShown;
	}

	std::size_t risksShown = 0;
	for (const auto& risk : analytics.legalRisks)
	{
		if (risksShown >= 3)
		{
			break;
		}

		stream << "- Известный риск: " << risk.title
			   << (risk.severity.empty() ? "" : " (" + risk.severity + ")") << "\n";
		++risksShown;
	}

	stream << "\nПодготовь: заголовок (2-3 строки), три инвестиционных тезиса "
			  "(Масштаб / Экономика / Создание стоимости), 2-3 риска, одну фразу-вывод по "
			  "оценке, 2-3 качественных шага создания стоимости без цифр, и одну строку "
			  "следующего шага сделки.";

	return stream.str();
}

OnePagerNarrative OnePagerNarrator::Compose(
	const CompanyAnalytics& analytics,
	const AnonymousIdentity& identity) const
{
	const PolzaAnswer answer = m_client->AskStructured(
		SystemPrompt(),
		BuildPrompt(analytics, identity),
		"one_pager_narrative",
		BuildSchema());

	nlohmann::json payload;
	try
	{
		payload = nlohmann::json::parse(answer.content);
	}
	catch (const nlohmann::json::parse_error&)
	{
		throw std::runtime_error("Не удалось разобрать текстовую часть одностраничника");
	}

	OnePagerNarrative narrative;
	narrative.headline = ReadText(payload, "headline");
	narrative.investmentCase = ReadPoints(payload, "investment_case");
	narrative.keyRisks = ReadPoints(payload, "key_risks");
	narrative.valuationTakeaway = ReadText(payload, "valuation_takeaway");
	narrative.valueCreationActions = ReadStrings(payload, "value_creation_actions");
	narrative.nextStep = ReadText(payload, "next_step");

	CapTo(narrative.investmentCase, MaxInvestmentCasePoints);
	CapTo(narrative.keyRisks, MaxRiskPoints);
	CapTo(narrative.valueCreationActions, MaxValueCreationActions);

	return narrative;
}

OnePagerNarrative OnePagerNarrator::BuildFallback(const CompanyAnalytics& analytics)
{
	OnePagerNarrative narrative;

	narrative.headline = "Актив с подтвержденными финансовыми показателями, доступный для дальнейшей проверки.";

	// Реальные риски уже берутся билдером напрямую из analytics.legalRisks (с приоритетом);
	// keyRisks оставляем пустым, чтобы не дублировать их вторым источником.
	narrative.nextStep = "NDA -> доступ к данным -> встреча с менеджментом -> индикативное предложение";

	return narrative;
}
