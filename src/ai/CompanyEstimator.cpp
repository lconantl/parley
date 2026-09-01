#include "CompanyEstimator.hpp"
#include "PolzaClient.hpp"
#include "finance/Metric.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{
constexpr std::size_t SearchResultCount = 8;
constexpr std::size_t MaxRequestedMetrics = 60;
constexpr double MinAcceptedConfidence = 0.05;
constexpr double MaxAcceptedConfidence = 0.85;

void AssertIsClientValid(const std::shared_ptr<PolzaClient>& client)
{
	if (client == nullptr)
	{
		throw std::invalid_argument("Клиент нейросети не может быть пустым");
	}
}

void AssertIsRequestValid(const EstimationRequest& request)
{
	if (request.identifier.empty() && request.name.empty())
	{
		throw std::invalid_argument("Для оценки нужен ИНН или название организации");
	}
}

std::string FormatNumber(const double value)
{
	std::ostringstream stream;
	stream.precision(2);
	stream << std::fixed << value;

	return stream.str();
}

std::string BuildIdentityBlock(const EstimationRequest& request)
{
	std::ostringstream stream;

	stream << "ИНН: " << (request.identifier.empty() ? "неизвестен" : request.identifier) << "\n";
	stream << "Название: " << (request.name.empty() ? "неизвестно" : request.name) << "\n";
	stream << "Организационно-правовая форма: " << request.legalForm << "\n";
	stream << "Основной вид деятельности: " << request.activity << "\n";
	stream << "Адрес: " << request.region << "\n";
	stream << "Сайт: " << request.website << "\n";
	stream << "Отчетный год: " << request.year << "\n";

	if (!request.brandSummary.empty())
	{
		stream << "Описание бизнеса: " << request.brandSummary << "\n";
	}

	return stream.str();
}

std::string BuildFactsBlock(const EstimationRequest& request)
{
	if (request.facts.empty())
	{
		return "Достоверных финансовых данных нет.\n";
	}

	std::ostringstream stream;
	stream << "Достоверные данные из отчетности, менять их нельзя:\n";

	for (const auto& fact : request.facts)
	{
		stream << "- " << fact.title
			   << " (" << fact.id << "): "
			   << FormatNumber(fact.value)
			   << " " << fact.unit << "\n";
	}

	return stream.str();
}

std::string BuildRequestsBlock(const EstimationRequest& request)
{
	std::ostringstream stream;
	stream << "Нужно оценить следующие показатели:\n";

	std::size_t counter = 0;
	for (const auto& item : request.requests)
	{
		if (counter >= MaxRequestedMetrics)
		{
			break;
		}

		stream << "- " << item.id << " — " << item.title << ", единица: " << item.unit;
		if (!item.hint.empty())
		{
			stream << ", причина отсутствия: " << item.hint;
		}
		stream << "\n";

		++counter;
	}

	return stream.str();
}

std::string ResearchSystemPrompt()
{
	return "Ты финансовый аналитик по российскому рынку. Ищи в открытых источниках сведения о "
		   "компании: чем занимается, масштаб, сегменты, клиенты, конкуренты, размер и рост рынка, "
		   "судебные и регуляторные проблемы, публичные оценки стоимости. Опирайся на российские "
		   "источники и отраслевые обзоры. Если компания не найдена, прямо напиши об этом. "
		   "Не выдумывай факты, помечай, где данные приблизительные. При указании денежных сумм используй сокращение «руб.», а не знак рубля.";
}

std::string ExtractionSystemPrompt()
{
	return "Ты финансовый аналитик по российскому рынку. Заполняешь пробелы в модели компании "
		   "приблизительными оценками. Правила: деньги указывай в рублях за год, проценты числом "
		   "процентов, коэффициенты кратностью. Опирайся на отраслевые нормы российского рынка, а "
		   "не на зарубежные бенчмарки: мультипликаторы EV/EBITDA в России обычно 3-6, а не 10-14. "
		   "Оценки должны быть согласованы с уже известными достоверными данными и между собой. "
		   "Если показатель оценить нельзя, поставь confidence 0. Уверенность выше 0.85 не ставь "
		   "никогда, это всегда приблизительная величина. При указании денежных сумм используй сокращение «руб.», а не знак рубля.";
}

nlohmann::json BuildNumericField(const char* description)
{
	return {{"type", "number"}, {"description", description}};
}

nlohmann::json BuildStringField(const char* description)
{
	return {{"type", "string"}, {"description", description}};
}

nlohmann::json BuildObjectSchema(const nlohmann::json& properties, const nlohmann::json& required)
{
	return {{"type", "object"},
		{"properties", properties},
		{"required", required},
		{"additionalProperties", false}};
}

nlohmann::json BuildArraySchema(const nlohmann::json& itemSchema)
{
	return {{"type", "array"}, {"items", itemSchema}};
}

nlohmann::json BuildEstimateSchema()
{
	return BuildObjectSchema(
		{{"id", BuildStringField("идентификатор показателя из списка")},
			{"value", BuildNumericField("оценка значения")},
			{"confidence", BuildNumericField("уверенность от 0 до 0.85")},
			{"basis", BuildStringField("на чем основана оценка")}},
		nlohmann::json::array({"id", "value", "confidence", "basis"}));
}

nlohmann::json BuildSegmentSchema()
{
	return BuildObjectSchema(
		{{"name", BuildStringField("название сегмента")},
			{"revenue", BuildNumericField("выручка сегмента в рублях")},
			{"profit", BuildNumericField("прибыль сегмента в рублях")},
			{"growth", BuildNumericField("рост сегмента в процентах")},
			{"confidence", BuildNumericField("уверенность от 0 до 0.85")}},
		nlohmann::json::array({"name", "revenue", "profit", "growth", "confidence"}));
}

nlohmann::json BuildCompetitorSchema()
{
	return BuildObjectSchema(
		{{"name", BuildStringField("название конкурента")},
			{"inn", BuildStringField("ИНН конкурента или пустая строка")},
			{"revenue", BuildNumericField("выручка в рублях")},
			{"revenue_growth", BuildNumericField("рост выручки в процентах")},
			{"margin", BuildNumericField("чистая маржа в процентах")},
			{"valuation", BuildNumericField("оценка стоимости в рублях")},
			{"confidence", BuildNumericField("уверенность от 0 до 0.85")}},
		nlohmann::json::array(
			{"name", "inn", "revenue", "revenue_growth", "margin", "valuation", "confidence"}));
}

nlohmann::json BuildOperationSchema()
{
	return BuildObjectSchema(
		{{"name", BuildStringField("название отраслевого показателя")},
			{"unit", BuildStringField("единица измерения")},
			{"value", BuildNumericField("значение")},
			{"confidence", BuildNumericField("уверенность от 0 до 0.85")}},
		nlohmann::json::array({"name", "unit", "value", "confidence"}));
}

nlohmann::json BuildRiskSchema()
{
	return BuildObjectSchema(
		{{"title", BuildStringField("краткая формулировка риска")},
			{"description", BuildStringField("пояснение")},
			{"severity", BuildStringField("low, medium или high")},
			{"source", BuildStringField("источник сведений")}},
		nlohmann::json::array({"title", "description", "severity", "source"}));
}

nlohmann::json BuildResponseSchema()
{
	return BuildObjectSchema(
		{{"company_found", {{"type", "boolean"}, {"description", "найдены ли сведения о компании"}}},
			{"estimates", BuildArraySchema(BuildEstimateSchema())},
			{"segments", BuildArraySchema(BuildSegmentSchema())},
			{"competitors", BuildArraySchema(BuildCompetitorSchema())},
			{"operations", BuildArraySchema(BuildOperationSchema())},
			{"risks", BuildArraySchema(BuildRiskSchema())},
			{"market_definition", BuildStringField("как определен рынок для оценки доли")},
			{"notes", BuildStringField("ключевые допущения одним абзацем")}},
		nlohmann::json::array({"company_found", "estimates", "segments", "competitors", "operations", "risks", "market_definition", "notes"}));
}

double ReadNumber(const nlohmann::json& node, const char* key)
{
	if (!node.contains(key) || !node.at(key).is_number())
	{
		return 0.0;
	}

	return node.at(key).get<double>();
}

std::string ReadText(const nlohmann::json& node, const char* key)
{
	if (!node.contains(key) || !node.at(key).is_string())
	{
		return {};
	}

	return node.at(key).get<std::string>();
}

bool ReadFlag(const nlohmann::json& node, const char* key)
{
	return node.contains(key) && node.at(key).is_boolean() && node.at(key).get<bool>();
}

double NormalizeConfidence(const double confidence)
{
	if (confidence < MinAcceptedConfidence)
	{
		return 0.0;
	}

	return confidence > MaxAcceptedConfidence ? MaxAcceptedConfidence : confidence;
}

MetricValue MakeEstimatedMetric(const double value, const double confidence, std::string basis)
{
	return Metric::Estimated(value, NormalizeConfidence(confidence), std::move(basis));
}

const nlohmann::json& ReadArray(const nlohmann::json& node, const char* key)
{
	static const nlohmann::json empty = nlohmann::json::array();

	if (!node.contains(key) || !node.at(key).is_array())
	{
		return empty;
	}

	return node.at(key);
}

nlohmann::json ParsePayload(const std::string& content)
{
	try
	{
		return nlohmann::json::parse(content);
	}
	catch (const nlohmann::json::parse_error&)
	{
		throw std::runtime_error("Нейросеть вернула ответ не в формате JSON");
	}
}

void ReadEstimates(const nlohmann::json& payload, EstimationResult& result)
{
	for (const auto& node : ReadArray(payload, "estimates"))
	{
		MetricEstimate estimate;
		estimate.id = ReadText(node, "id");
		estimate.value = ReadNumber(node, "value");
		estimate.confidence = NormalizeConfidence(ReadNumber(node, "confidence"));
		estimate.basis = ReadText(node, "basis");

		if (estimate.id.empty() || estimate.confidence <= 0.0)
		{
			continue;
		}

		result.estimates.push_back(std::move(estimate));
	}
}

void ReadSegments(const nlohmann::json& payload, EstimationResult& result)
{
	for (const auto& node : ReadArray(payload, "segments"))
	{
		const std::string name = ReadText(node, "name");
		if (name.empty())
		{
			continue;
		}

		const double confidence = ReadNumber(node, "confidence");
		const std::string basis = "оценка по открытым источникам";

		BusinessSegment segment;
		segment.name = name;
		segment.revenue = MakeEstimatedMetric(ReadNumber(node, "revenue"), confidence, basis);
		segment.profit = MakeEstimatedMetric(ReadNumber(node, "profit"), confidence, basis);
		segment.growth = MakeEstimatedMetric(ReadNumber(node, "growth"), confidence, basis);

		result.segments.push_back(std::move(segment));
	}
}

void ReadCompetitors(const nlohmann::json& payload, EstimationResult& result)
{
	for (const auto& node : ReadArray(payload, "competitors"))
	{
		const std::string name = ReadText(node, "name");
		if (name.empty())
		{
			continue;
		}

		const double confidence = ReadNumber(node, "confidence");
		const std::string basis = "оценка по открытым источникам";

		Competitor competitor;
		competitor.name = name;
		competitor.inn = ReadText(node, "inn");
		competitor.revenue = MakeEstimatedMetric(ReadNumber(node, "revenue"), confidence, basis);
		competitor.revenueGrowth = MakeEstimatedMetric(
			ReadNumber(node, "revenue_growth"), confidence, basis);
		competitor.margin = MakeEstimatedMetric(ReadNumber(node, "margin"), confidence, basis);
		competitor.valuation = MakeEstimatedMetric(
			ReadNumber(node, "valuation"), confidence, basis);

		result.competitors.push_back(std::move(competitor));
	}
}

void ReadOperations(const nlohmann::json& payload, EstimationResult& result)
{
	for (const auto& node : ReadArray(payload, "operations"))
	{
		const std::string name = ReadText(node, "name");
		if (name.empty())
		{
			continue;
		}

		OperationalIndicator indicator;
		indicator.name = name;
		indicator.unit = ReadText(node, "unit");
		indicator.value = MakeEstimatedMetric(
			ReadNumber(node, "value"),
			ReadNumber(node, "confidence"),
			"отраслевая оценка");

		result.operations.push_back(std::move(indicator));
	}
}

void ReadRisks(const nlohmann::json& payload, EstimationResult& result)
{
	for (const auto& node : ReadArray(payload, "risks"))
	{
		const std::string title = ReadText(node, "title");
		if (title.empty())
		{
			continue;
		}

		result.legalRisks.push_back({title,
			ReadText(node, "description"),
			ReadText(node, "severity"),
			ReadText(node, "source")});
	}
}
} // namespace

CompanyEstimator::CompanyEstimator(std::shared_ptr<PolzaClient> client)
	: m_client(std::move(client))
{
	AssertIsClientValid(m_client);
}

EstimationRequest CompanyEstimator::BuildRequest(
	const CompanyAnalytics& analytics,
	const MetricIndex& index)
{
	EstimationRequest request;
	request.identifier = analytics.identifier;
	request.name = analytics.name;
	request.activity = analytics.activity;
	request.region = analytics.region;
	request.legalForm = analytics.legalForm;
	request.year = analytics.year;

	for (const auto& descriptor : index.ListAll())
	{
		const MetricValue& metric = *descriptor.value;

		if (Metric::IsMissing(metric))
		{
			request.requests.push_back({descriptor.id,
				descriptor.title,
				Metric::DescribeUnit(descriptor.unit),
				metric.comment});
			continue;
		}

		if (metric.origin == MetricOrigin::Reported)
		{
			request.facts.push_back({descriptor.id,
				descriptor.title,
				Metric::DescribeUnit(descriptor.unit),
				metric.value});
		}
	}

	return request;
}

EstimationResult CompanyEstimator::Estimate(const EstimationRequest& request) const
{
	AssertIsRequestValid(request);

	double cost = 0.0;
	const std::string notes = Research(request, cost);
	EstimationResult result = ExtractStructured(request, notes, cost);

	result.researchNotes = notes;
	result.costRub = cost;

	return result;
}

std::string CompanyEstimator::Research(const EstimationRequest& request, double& cost) const
{
	std::ostringstream prompt;
	prompt << "Собери сведения о российской компании.\n\n"
		   << BuildIdentityBlock(request) << "\n"
		   << BuildFactsBlock(request) << "\n"
		   << "Интересует: чем зарабатывает, сегменты выручки, кто клиенты и насколько "
			  "концентрирована база, конкуренты и их масштаб, размер и динамика рынка, "
			  "отраслевые операционные метрики, судебные и регуляторные проблемы, "
			  "публичные оценки стоимости и отраслевые мультипликаторы.\n"
		   << "Если такой компании не существует, напиши об этом первой строкой.";

	const PolzaAnswer answer = m_client->AskWithSearch(
		ResearchSystemPrompt(),
		prompt.str(),
		SearchResultCount);

	cost += answer.costRub;

	return answer.content;
}

EstimationResult CompanyEstimator::ExtractStructured(
	const EstimationRequest& request,
	const std::string& notes,
	double& cost) const
{
	std::ostringstream prompt;
	prompt << BuildIdentityBlock(request) << "\n"
		   << BuildFactsBlock(request) << "\n"
		   << BuildRequestsBlock(request) << "\n"
		   << "Результаты поиска:\n"
		   << notes << "\n\n"
		   << "Заполни оценками только те показатели, которые перечислены выше. "
			  "Идентификаторы менять нельзя.";

	const PolzaAnswer answer = m_client->AskStructured(
		ExtractionSystemPrompt(),
		prompt.str(),
		"company_estimates",
		BuildResponseSchema());

	cost += answer.costRub;

	const nlohmann::json payload = ParsePayload(answer.content);

	EstimationResult result;
	result.companyFound = ReadFlag(payload, "company_found");
	result.marketDefinition = ReadText(payload, "market_definition");

	ReadEstimates(payload, result);
	ReadSegments(payload, result);
	ReadCompetitors(payload, result);
	ReadOperations(payload, result);
	ReadRisks(payload, result);

	return result;
}

void CompanyEstimator::Apply(
	const EstimationResult& result,
	MetricIndex& index,
	CompanyAnalytics& analytics)
{
	for (const auto& estimate : result.estimates)
	{
		index.Apply(estimate.id, MakeEstimatedMetric(estimate.value, estimate.confidence, estimate.basis));
	}

	for (const auto& segment : result.segments)
	{
		analytics.segments.push_back(segment);
	}

	for (const auto& competitor : result.competitors)
	{
		analytics.competitors.push_back(competitor);
	}

	for (const auto& operation : result.operations)
	{
		analytics.operations.push_back(operation);
	}

	for (const auto& risk : result.legalRisks)
	{
		analytics.legalRisks.push_back(risk);
	}

	if (!result.marketDefinition.empty())
	{
		analytics.market.definition = result.marketDefinition;
	}

	analytics.researchNotes = result.researchNotes;
	analytics.estimationCost += result.costRub;
}