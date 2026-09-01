#include "DueDiligenceNarrator.hpp"

#include "finance/Metric.hpp"
#include "finance/MetricFormatter.hpp"
#include "finance/MetricReport.hpp"

#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{
constexpr std::size_t MinSummaryPoints = 5;
constexpr std::size_t MaxSummaryPoints = 7;

void AssertIsClientValid(const std::shared_ptr<PolzaClient>& client)
{
	if (client == nullptr)
	{
		throw std::invalid_argument("Клиент нейросети не может быть пустым");
	}
}

std::string SystemPrompt()
{
	return "Ты аналитик инвестиционного due diligence по российскому рынку. Пишешь разделы "
		   "отчета для инвестора в сдержанном консалтинговом стиле: короткие утверждения, "
		   "конкретные цифры, никакой рекламы и никаких восклицаний.\n"
		   "Жесткие правила:\n"
		   "1. Объект анализа называется только «Целевая компания». Настоящее название, ИНН, "
		   "имена собственников и руководителей упоминать запрещено.\n"
		   "2. Конкурентов называй обезличенно: «Конкурент A», «Конкурент B».\n"
		   "3. Не упоминай источники данных, методику, инструменты анализа и то, кем или чем "
		   "подготовлен текст.\n"
		   "4. Не пиши о вероятностях, уверенности, точности и приблизительности оценок. "
		   "Формулируй утверждения прямо.\n"
		   "5. Если показателя не хватает для вывода, не додумывай его молча: вынеси вопрос в "
		   "список того, что нужно проверить дополнительно.\n"
		   "6. Каждое утверждение должно опираться на переданные цифры."
		   "7. Никогда не используй символ рубля (₽), всегда пиши текстовое сокращение «руб.».";
}

nlohmann::json StringArray(const char* description)
{
	return {{"type", "array"},
		{"items", {{"type", "string"}}},
		{"description", description}};
}

nlohmann::json TextField(const char* description)
{
	return {{"type", "string"}, {"description", description}};
}

nlohmann::json PointArray(const char* description)
{
	return {{"type", "array"},
		{"items",
			{{"type", "object"},
				{"properties",
					{{"title", TextField("краткая формулировка, до шести слов")},
						{"body", TextField("два-три предложения с опорой на цифры")}}},
				{"required", nlohmann::json::array({"title", "body"})},
				{"additionalProperties", false}}},
		{"description", description}};
}

nlohmann::json BuildSchema()
{
	nlohmann::json properties = {
		{"executive_summary", StringArray("от 5 до 7 главных выводов, каждый одно предложение")},
		{"investment_verdict", TextField("общий инвестиционный вывод, два предложения")},
		{"business_profile", TextField("чем занимается бизнес, масштаб, география, абзац")},
		{"positioning", TextField("позиционирование на рынке, абзац")},
		{"market_commentary", TextField("размер и динамика рынка, место компании, абзац")},
		{"valuation_commentary", TextField("оценка стоимости и мультипликаторы, абзац")},
		{"risks", PointArray("от 4 до 6 ключевых рисков")},
		{"opportunities", PointArray("от 3 до 5 возможностей роста")},
		{"open_questions", StringArray("от 5 до 8 вопросов, которые нужно проверить")},
		{"next_steps", StringArray("от 3 до 5 конкретных следующих шагов")},
		{"conclusion", TextField("инвестиционный тезис, три-четыре предложения")},
		{"revenue_takeaway", TextField("вывод по динамике выручки, одно предложение")},
		{"margin_takeaway", TextField("вывод по маржинальности, одно предложение")},
		{"cash_flow_takeaway", TextField("вывод по денежному потоку, одно предложение")},
		{"balance_takeaway", TextField("вывод по балансу и долгу, одно предложение")},
		{"quality_takeaway", TextField("вывод по качеству прибыли, одно предложение")},
		{"returns_takeaway", TextField("вывод по эффективности капитала, одно предложение")},
		{"competition_takeaway", TextField("вывод по конкурентному окружению, одно предложение")}};

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

std::string BuildMetricBlock(const CompanyAnalytics& analytics)
{
	MetricFormatOptions options;
	options.showOrigin = false;
	options.showConfidence = false;
	options.showMissing = false;
	options.showNotApplicable = false;

	const MetricFormatter formatter(options);
	const MetricReport report(analytics);

	std::ostringstream stream;

	for (const MetricGroup group : MetricReport::ListGroups())
	{
		const auto rows = report.GetGroup(group);
		bool printedTitle = false;

		for (const auto& row : rows)
		{
			if (!Metric::IsKnown(row.value))
			{
				continue;
			}

			if (!printedTitle)
			{
				stream << "\n"
					   << MetricReport::DescribeGroup(group) << ":\n";
				printedTitle = true;
			}

			stream << "- " << row.title << ": "
				   << formatter.FormatValue(row.value, row.unit) << "\n";
		}
	}

	return stream.str();
}

std::string BuildSeriesBlock(const CompanyAnalytics& analytics)
{
	if (analytics.series.empty())
	{
		return {};
	}

	MetricFormatOptions options;
	options.showOrigin = false;
	options.showConfidence = false;

	const MetricFormatter formatter(options);

	std::ostringstream stream;
	stream << "\nДинамика по годам:\n";

	for (const auto& snapshot : analytics.series)
	{
		stream << "- " << snapshot.year
			   << ": выручка " << formatter.FormatValue(snapshot.revenue, MetricUnit::Money)
			   << ", EBITDA " << formatter.FormatValue(snapshot.ebitda, MetricUnit::Money)
			   << ", чистая прибыль "
			   << formatter.FormatValue(snapshot.netProfit, MetricUnit::Money)
			   << ", FCF " << formatter.FormatValue(snapshot.freeCashFlow, MetricUnit::Money)
			   << "\n";
	}

	return stream.str();
}

std::string BuildGapBlock(const CompanyAnalytics& analytics)
{
	const MetricReport report(analytics);

	std::ostringstream stream;
	stream << "\nПоказатели без подтверждения отчетностью:\n";

	std::size_t counter = 0;
	for (const auto& row : report.GetRows())
	{
		const bool weak = row.value.origin == MetricOrigin::Unavailable
			|| (Metric::IsKnown(row.value) && row.value.confidence < 0.6);

		if (!weak || counter >= 15)
		{
			continue;
		}

		stream << "- " << row.title << "\n";
		++counter;
	}

	return counter == 0 ? std::string() : stream.str();
}
} // namespace

DueDiligenceNarrator::DueDiligenceNarrator(std::shared_ptr<PolzaClient> client)
	: m_client(std::move(client))
{
	AssertIsClientValid(m_client);
}

std::string DueDiligenceNarrator::BuildPrompt(
	const CompanyAnalytics& analytics,
	const AnonymousIdentity& identity) const
{
	std::ostringstream stream;

	stream << "Подготовь разделы отчета due diligence.\n\n"
		   << "Объект: " << identity.subject << "\n"
		   << "Отрасль: " << identity.industry << "\n"
		   << "Регион: " << identity.region << "\n"
		   << "Масштаб: " << identity.scale << "\n"
		   << "Период анализа: " << identity.period << "\n";

	if (!analytics.market.definition.empty())
	{
		stream << "Рынок: " << analytics.market.definition << "\n";
	}

	stream << BuildMetricBlock(analytics)
		   << BuildSeriesBlock(analytics)
		   << BuildGapBlock(analytics);

	stream << "\nВыводы по каждому разделу должны быть в одно предложение и опираться на "
			  "приведенные цифры. Сводка должна содержать от "
		   << MinSummaryPoints << " до " << MaxSummaryPoints << " пунктов.";

	return stream.str();
}

DueDiligenceNarrative DueDiligenceNarrator::Compose(
	const CompanyAnalytics& analytics,
	const AnonymousIdentity& identity) const
{
	const PolzaAnswer answer = m_client->AskStructured(
		SystemPrompt(),
		BuildPrompt(analytics, identity),
		"due_diligence_narrative",
		BuildSchema());

	nlohmann::json payload;
	try
	{
		payload = nlohmann::json::parse(answer.content);
	}
	catch (const nlohmann::json::parse_error&)
	{
		throw std::runtime_error("Не удалось разобрать текстовую часть отчета");
	}

	DueDiligenceNarrative narrative;
	narrative.executiveSummary = ReadStrings(payload, "executive_summary");
	narrative.investmentVerdict = ReadText(payload, "investment_verdict");
	narrative.businessProfile = ReadText(payload, "business_profile");
	narrative.positioning = ReadText(payload, "positioning");
	narrative.marketCommentary = ReadText(payload, "market_commentary");
	narrative.valuationCommentary = ReadText(payload, "valuation_commentary");
	narrative.risks = ReadPoints(payload, "risks");
	narrative.opportunities = ReadPoints(payload, "opportunities");
	narrative.openQuestions = ReadStrings(payload, "open_questions");
	narrative.nextSteps = ReadStrings(payload, "next_steps");
	narrative.conclusion = ReadText(payload, "conclusion");
	narrative.revenueTakeaway = ReadText(payload, "revenue_takeaway");
	narrative.marginTakeaway = ReadText(payload, "margin_takeaway");
	narrative.cashFlowTakeaway = ReadText(payload, "cash_flow_takeaway");
	narrative.balanceTakeaway = ReadText(payload, "balance_takeaway");
	narrative.qualityTakeaway = ReadText(payload, "quality_takeaway");
	narrative.returnsTakeaway = ReadText(payload, "returns_takeaway");
	narrative.competitionTakeaway = ReadText(payload, "competition_takeaway");

	return narrative;
}

DueDiligenceNarrative DueDiligenceNarrator::BuildFallback(const CompanyAnalytics& analytics)
{
	DueDiligenceNarrative narrative;
	narrative.businessProfile = "Описание бизнеса подготовлено на основе регистрационных данных "
								"и финансовой отчетности.";
	narrative.conclusion = "Решение требует дополнительной проверки: часть разделов отчета "
						   "заполнена только количественными показателями.";

	const MetricReport report(analytics);

	for (const auto& row : report.GetRows())
	{
		if (row.value.origin == MetricOrigin::Unavailable && narrative.openQuestions.size() < 8)
		{
			narrative.openQuestions.push_back("Запросить у компании: " + row.title);
		}
	}

	for (const auto& risk : analytics.legalRisks)
	{
		narrative.risks.push_back({risk.title, risk.description});
	}

	return narrative;
}