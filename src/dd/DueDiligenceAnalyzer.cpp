#include "DueDiligenceAnalyzer.hpp"

#include "ai/PolzaAiClient.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

namespace
{

constexpr double EQUITY_RELIABILITY_MINIMUM = 1000.0;
constexpr double ACCOUNTING_EQUATION_TOLERANCE = 1.0;

std::optional<double> JsonNumber(
	const nlohmann::json& block,
	const char* key)
{
	if (!block.is_object())
	{
		return std::nullopt;
	}

	const auto iterator = block.find(key);

	if (
		iterator == block.end()
		|| iterator->is_null()
		|| !iterator->is_number())
	{
		return std::nullopt;
	}

	return iterator->get<double>();
}

DueDiligenceMetric MakeUnavailable(
	const std::string& name,
	const std::string& reason)
{
	return {name, std::nullopt, "UNAVAILABLE", reason};
}

DueDiligenceMetric MakeAvailable(
	const std::string& name,
	const double value)
{
	return {name, value, "AVAILABLE", std::nullopt};
}

std::string FormatMetricLine(
	const DueDiligenceMetric& metric)
{
	std::ostringstream output;

	output << "- **" << metric.name << "**: ";

	if (metric.value)
	{
		output.precision(4);
		output << *metric.value;
	}
	else
	{
		output << "н/д";
	}

	output << " _(" << metric.availability << ")_";

	if (metric.reason)
	{
		output << " — " << *metric.reason;
	}

	return output.str();
}

std::string SeverityIcon(
	const std::string& severity)
{
	if (severity == "CRITICAL")
	{
		return "🟥";
	}

	if (severity == "HIGH")
	{
		return "🟧";
	}

	if (severity == "MEDIUM")
	{
		return "🟨";
	}

	if (severity == "LOW")
	{
		return "🟦";
	}

	return "⬜";
}

std::string AddMonthsToIsoDate(
	const std::string& isoDate,
	const int months)
{
	if (isoDate.size() < 10)
	{
		return isoDate;
	}

	int year = std::stoi(isoDate.substr(0, 4));
	int month = std::stoi(isoDate.substr(5, 2));
	const std::string day = isoDate.substr(8, 2);

	month += months;

	while (month > 12)
	{
		month -= 12;
		++year;
	}

	std::ostringstream output;

	output
		<< year << '-'
		<< (month < 10 ? "0" : "") << month << '-'
		<< day;

	return output.str();
}

} // namespace

DueDiligenceAnalyzer::DueDiligenceAnalyzer(
	BfoOrganizationProfile profile,
	std::vector<BfoPeriodReport> history,
	std::unordered_map<std::string, BfoDetailBreakdown> latestPeriodDetails,
	const PolzaAiClient& aiClient)
	: m_profile(std::move(profile))
	, m_history(std::move(history))
	, m_latestPeriodDetails(std::move(latestPeriodDetails))
	, m_aiClient(aiClient)
{
}

const BfoPeriodReport* DueDiligenceAnalyzer::FindLatestPeriodWithData() const
{
	const BfoPeriodReport* latest = nullptr;

	for (const BfoPeriodReport& report : m_history)
	{
		if (report.typeCorrections.empty())
		{
			continue;
		}

		if (
			latest == nullptr
			|| report.period > latest->period)
		{
			latest = &report;
		}
	}

	return latest;
}

const BfoCorrection* DueDiligenceAnalyzer::FindPrimaryCorrection(
	const BfoPeriodReport& period) const
{
	if (period.typeCorrections.empty())
	{
		return nullptr;
	}

	return &period.typeCorrections.front().correction;
}

std::vector<DueDiligenceMetric> DueDiligenceAnalyzer::ComputeRevenueGrowthSeries() const
{
	std::vector<BfoPeriodReport> sortedHistory = m_history;

	std::sort(
		sortedHistory.begin(),
		sortedHistory.end(),
		[](const BfoPeriodReport& lhs, const BfoPeriodReport& rhs) {
			return lhs.period < rhs.period;
		});

	std::vector<DueDiligenceMetric> result;

	for (std::size_t index = 1; index < sortedHistory.size(); ++index)
	{
		const std::string metricName = "Revenue growth " + sortedHistory[index - 1].period
			+ " → " + sortedHistory[index].period;

		const std::optional<double>& previousRevenue = sortedHistory[index - 1].gainSum;
		const std::optional<double>& currentRevenue = sortedHistory[index].gainSum;

		if (!previousRevenue || !currentRevenue)
		{
			result.push_back(
				MakeUnavailable(metricName, "выручка недоступна за один из периодов"));
			continue;
		}

		if (*previousRevenue == 0.0)
		{
			result.push_back(
				{metricName, std::nullopt, "UNDEFINED_ZERO_BASE", "база сравнения равна нулю"});
			continue;
		}

		const double growth = (*currentRevenue - *previousRevenue) / std::abs(*previousRevenue);

		result.push_back(MakeAvailable(metricName, growth));
	}

	return result;
}

DueDiligenceMetric DueDiligenceAnalyzer::ComputeCagr() const
{
	std::vector<BfoPeriodReport> sortedHistory = m_history;

	std::sort(
		sortedHistory.begin(),
		sortedHistory.end(),
		[](const BfoPeriodReport& lhs, const BfoPeriodReport& rhs) {
			return lhs.period < rhs.period;
		});

	if (sortedHistory.size() < 2)
	{
		return MakeUnavailable("CAGR", "недостаточно периодов");
	}

	const std::optional<double>& startValue = sortedHistory.front().gainSum;
	const std::optional<double>& endValue = sortedHistory.back().gainSum;

	if (
		!startValue
		|| !endValue
		|| *startValue <= 0.0
		|| *endValue < 0.0)
	{
		return MakeUnavailable("CAGR", "начальное значение должно быть положительным");
	}

	int years = 0;

	try
	{
		years = std::stoi(sortedHistory.back().period) - std::stoi(sortedHistory.front().period);
	}
	catch (...)
	{
		return MakeUnavailable("CAGR", "не удалось определить число лет");
	}

	if (years <= 0)
	{
		return MakeUnavailable("CAGR", "недостаточный временной интервал");
	}

	const double cagr = std::pow(*endValue / *startValue, 1.0 / years) - 1.0;

	return MakeAvailable("CAGR", cagr);
}

std::vector<DueDiligenceMetric> DueDiligenceAnalyzer::ComputeLatestPeriodMetrics(
	const BfoCorrection& correction) const
{
	std::vector<DueDiligenceMetric> result;

	const nlohmann::json& balance = correction.balance;
	const nlohmann::json& financialResult = correction.financialResult;

	const std::optional<double> revenue = JsonNumber(financialResult, "current2110");
	const std::optional<double> netIncome = JsonNumber(financialResult, "current2400");

	if (revenue && netIncome && *revenue > 0.0)
	{
		result.push_back(MakeAvailable("Net margin", *netIncome / *revenue));
	}
	else
	{
		result.push_back(MakeUnavailable("Net margin", "выручка недоступна или неположительна"));
	}

	const std::optional<double> currentAssets = JsonNumber(balance, "current1600");
	const std::optional<double> previousAssets = JsonNumber(balance, "previous1600");

	if (netIncome && currentAssets && previousAssets)
	{
		result.push_back(MakeAvailable("ROA", *netIncome / ((*currentAssets + *previousAssets) / 2.0)));
	}
	else if (netIncome && currentAssets)
	{
		result.push_back(
			{"roa_end_assets_estimate", *netIncome / *currentAssets, "PARTIAL", "нет данных за предыдущий период, использованы только активы на конец периода"});
	}
	else
	{
		result.push_back(MakeUnavailable("ROA", "недостаточно данных по активам или прибыли"));
	}

	const std::optional<double> currentEquity = JsonNumber(balance, "current1300");
	const std::optional<double> previousEquity = JsonNumber(balance, "previous1300");

	if (netIncome && currentEquity && previousEquity)
	{
		const double averageEquity = (*currentEquity + *previousEquity) / 2.0;

		if (std::abs(averageEquity) < EQUITY_RELIABILITY_MINIMUM)
		{
			result.push_back(
				{"ROE", std::nullopt, "UNRELIABLE_DENOMINATOR", "средний капитал слишком близок к нулю"});
		}
		else
		{
			result.push_back(MakeAvailable("ROE", *netIncome / averageEquity));
		}
	}
	else
	{
		result.push_back(MakeUnavailable("ROE", "недостаточно данных по капиталу или прибыли"));
	}

	const std::optional<double> currentAssetsShort = JsonNumber(balance, "current1200");
	const std::optional<double> shortTermLiabilities = JsonNumber(balance, "current1500");

	if (currentAssetsShort && shortTermLiabilities && *shortTermLiabilities != 0.0)
	{
		result.push_back(MakeAvailable("Current ratio", *currentAssetsShort / *shortTermLiabilities));
	}
	else
	{
		result.push_back(MakeUnavailable("Current ratio", "краткосрочные обязательства недоступны или равны нулю"));
	}

	if (currentEquity && currentAssets && *currentAssets != 0.0)
	{
		result.push_back(MakeAvailable("Equity ratio", *currentEquity / *currentAssets));
	}
	else
	{
		result.push_back(MakeUnavailable("Equity ratio", "недостаточно данных по капиталу или активам"));
	}

	const std::optional<double> longTermLiabilities = JsonNumber(balance, "current1400");

	if (longTermLiabilities && shortTermLiabilities && currentEquity && *currentEquity != 0.0)
	{
		const double totalLiabilities = *longTermLiabilities + *shortTermLiabilities;

		result.push_back(MakeAvailable("Total liabilities / Equity", totalLiabilities / *currentEquity));

		if (currentAssets && *currentAssets != 0.0)
		{
			result.push_back(MakeAvailable("Total liabilities / Assets", totalLiabilities / *currentAssets));
		}
		else
		{
			result.push_back(MakeUnavailable("Total liabilities / Assets", "активы недоступны"));
		}
	}
	else
	{
		result.push_back(MakeUnavailable("Total liabilities / Equity", "недостаточно данных по обязательствам или капиталу"));
		result.push_back(MakeUnavailable("Total liabilities / Assets", "недостаточно данных по обязательствам или активам"));
	}

	result.push_back(
		MakeUnavailable(
			"Net debt",
			"нет разбивки на процентные (заёмные) и прочие обязательства — "
			"total_liabilities не равно interest-bearing debt"));

	return result;
}

std::vector<DueDiligenceFlag> DueDiligenceAnalyzer::DetectFlags(
	const BfoPeriodReport& latestPeriod,
	const BfoCorrection& correction) const
{
	std::vector<DueDiligenceFlag> flags;

	const nlohmann::json& balance = correction.balance;
	const nlohmann::json& financialResult = correction.financialResult;

	const std::optional<double> equity = JsonNumber(balance, "current1300");

	if (equity && *equity < 0.0)
	{
		flags.push_back(
			{"NEGATIVE_EQUITY", "HIGH", "FINANCIAL", "Собственный капитал отрицателен на конец отчётного периода."});
	}

	const std::optional<double> assetsTotal = JsonNumber(balance, "current1600");
	const std::optional<double> liabilitiesTotal = JsonNumber(balance, "current1700");

	if (
		assetsTotal
		&& liabilitiesTotal
		&& std::abs(*assetsTotal - *liabilitiesTotal) > ACCOUNTING_EQUATION_TOLERANCE)
	{
		flags.push_back(
			{"DATA_INCONSISTENCY", "MEDIUM", "FINANCIAL", "Баланс актива и пассива (1600 vs 1700) не сходится в пределах допуска."});
	}

	if (correction.datePresent && correction.datePresent->size() >= 10)
	{
		const std::string periodEnd = latestPeriod.period + "-12-31";
		const std::string deadline = AddMonthsToIsoDate(periodEnd, 3);
		const std::string presentedDate = correction.datePresent->substr(0, 10);

		if (presentedDate > deadline)
		{
			flags.push_back(
				{"DEADLINE_VIOLATION", "MEDIUM", "GOVERNANCE", "Отчётность представлена " + presentedDate + ", позднее трёх месяцев после окончания отчётного периода (" + deadline + ")."});
		}
	}

	const std::optional<double> currentRevenue = JsonNumber(financialResult, "current2110");
	const std::optional<double> previousRevenue = JsonNumber(financialResult, "previous2110");
	const std::optional<double> currentNetIncome = JsonNumber(financialResult, "current2400");
	const std::optional<double> previousNetIncome = JsonNumber(financialResult, "previous2400");

	if (
		currentRevenue && previousRevenue && *previousRevenue != 0.0
		&& currentNetIncome && previousNetIncome && *previousNetIncome != 0.0)
	{
		const double revenueGrowth = (*currentRevenue - *previousRevenue) / std::abs(*previousRevenue);
		const double incomeGrowth = (*currentNetIncome - *previousNetIncome) / std::abs(*previousNetIncome);

		if (incomeGrowth - revenueGrowth > 1.0)
		{
			flags.push_back(
				{"EARNINGS_QUALITY_WATCH", "LOW", "FINANCIAL", "Чистая прибыль растёт значительно быстрее выручки — требует проверки качества прибыли."});
		}
	}

	return flags;
}

nlohmann::json DueDiligenceAnalyzer::BuildAiContext(
	const std::vector<DueDiligenceMetric>& metrics,
	const std::vector<DueDiligenceFlag>& flags) const
{
	nlohmann::json metricsJson = nlohmann::json::array();

	for (const DueDiligenceMetric& metric : metrics)
	{
		metricsJson.push_back(
			{{"name", metric.name},
				{"value", metric.value ? nlohmann::json(*metric.value) : nlohmann::json(nullptr)},
				{"availability", metric.availability}});
	}

	nlohmann::json flagsJson = nlohmann::json::array();

	for (const DueDiligenceFlag& flag : flags)
	{
		flagsJson.push_back(
			{{"code", flag.code}, {"severity", flag.severity}, {"dimension", flag.dimension}});
	}

	return {
		{"entity",
			{{"inn", m_profile.inn},
				{"short_name", m_profile.shortName},
				{"okved", m_profile.okved2 ? m_profile.okved2->name : ""},
				{"status", m_profile.statusCode.value_or("")},
				{"registration_date", m_profile.registrationDate.value_or("")}}},
		{"financial_metrics", metricsJson},
		{"financial_flags", flagsJson},
		{"available_sources", {"bo.nalog.gov.ru (публичная бухгалтерская отчётность)"}},
		{"unavailable_sources",
			{"ЕГРЮЛ/ЕГРИП", "Картотека арбитражных дел", "ФССП (исполнительные производства)", "Федресурс/банкротство", "Публичные закупки", "Веб-разведка"}},
	};
}

nlohmann::json DueDiligenceAnalyzer::RequestAiEnrichment(
	const nlohmann::json& context) const
{
	const std::string systemPrompt = "Ты — ассистент для первичного публичного due diligence российских компаний. "
									 "Тебе передан JSON со СТРУКТУРИРОВАННЫМИ реальными фактами и метриками, посчитанными "
									 "из официальной бухгалтерской отчётности (bo.nalog.gov.ru). У тебя НЕТ доступа к ЕГРЮЛ, "
									 "картотеке арбитражных дел, ФССП, Федресурсу, закупкам и вебу — они перечислены в "
									 "unavailable_sources. Твоя задача — вернуть СТРОГО валидный JSON (без markdown, без "
									 "пояснений вне JSON) со следующими полями (все — строки на русском языке, кроме "
									 "next_actions): legal_assessment, governance_assessment, business_consistency_assessment, "
									 "bankruptcy_enforcement_assessment, executive_summary, next_actions (массив строк). "
									 "Каждая оценка должна быть ОСТОРОЖНОЙ, в сослагательном наклонении, явно указывать, что "
									 "это не факт, а эвристическая оценка по косвенным признакам (масштаб бизнеса, отрасль, "
									 "устойчивость показателей), и не должна утверждать, что тебе известны судебные дела, "
									 "банкротство или нарушения — если о них ничего не известно, так и напиши.";

	try
	{
		return m_aiClient.CompleteJson(systemPrompt, context.dump());
	}
	catch (const std::exception& exception)
	{
		std::cerr << "[DD][AI] " << exception.what() << std::endl;

		return {{"unavailable", true}, {"reason", exception.what()}};
	}
}

std::string DueDiligenceAnalyzer::RenderMarkdown(
	const std::vector<DueDiligenceMetric>& revenueSeries,
	const DueDiligenceMetric& cagr,
	const std::vector<DueDiligenceMetric>& latestMetrics,
	const std::vector<DueDiligenceFlag>& flags,
	const nlohmann::json& aiResult) const
{
	std::ostringstream output;

	const bool aiAvailable = !aiResult.value("unavailable", false);
	const std::string& aiModel = m_aiClient.GetModel();

	output << "# Public Due Diligence — " << m_profile.fullName.value_or(m_profile.shortName) << "\n\n";

	output << "## 1. Entity\n\n";
	output << "- ИНН: " << m_profile.inn << "\n";
	output << "- ОГРН: " << m_profile.ogrn << "\n";

	if (m_profile.kpp)
	{
		output << "- КПП: " << *m_profile.kpp << "\n";
	}

	output << "- Статус: " << m_profile.statusCode.value_or("н/д") << " (с " << m_profile.statusDate.value_or("н/д") << ")\n";
	output << "- Дата регистрации: " << m_profile.registrationDate.value_or("н/д") << "\n";

	if (m_profile.okved2)
	{
		output << "- ОКВЭД: " << m_profile.okved2->id << " — " << m_profile.okved2->name << "\n";
	}

	if (m_profile.okopf)
	{
		output << "- ОКОПФ: " << m_profile.okopf->id << " — " << m_profile.okopf->name << "\n";
	}

	output << "\n## 2. Executive Decision\n\n";

	int highOrCriticalCount = 0;

	for (const DueDiligenceFlag& flag : flags)
	{
		if (flag.severity == "CRITICAL" || flag.severity == "HIGH")
		{
			++highOrCriticalCount;
		}
	}

	std::string decision = "NO_CRITICAL_FLAGS_OBSERVED";

	if (highOrCriticalCount > 0)
	{
		decision = "ENHANCED_DD_REQUIRED";
	}

	output << "- **Decision**: " << decision << "\n";
	output << "- **Financial risk flags**: " << flags.size() << " (из них HIGH/CRITICAL: " << highOrCriticalCount << ")\n";
	output << "- **Coverage**: financial = 1.0 (реальные данные bo.nalog.gov.ru); "
		   << "legal = governance(частично) = business = bankruptcy = enforcement = 0.0 (источников нет, только AI-эвристика ниже)\n";

	if (aiAvailable)
	{
		output << "\n"
			   << aiResult.value("executive_summary", std::string()) << "\n";
	}

	output << "\n## 3. Critical Findings\n\n";

	if (highOrCriticalCount == 0)
	{
		output << "Критических или высоких флагов по финансовым данным не обнаружено.\n";
	}
	else
	{
		for (const DueDiligenceFlag& flag : flags)
		{
			if (flag.severity == "CRITICAL" || flag.severity == "HIGH")
			{
				output << "- " << SeverityIcon(flag.severity) << " **" << flag.code << "**: " << flag.explanation << "\n";
			}
		}
	}

	output << "\n## 4. Legal\n\n";
	output << (aiAvailable ? aiResult.value("legal_assessment", std::string("н/д")) : "AI недоступен, оценка не сгенерирована.") << "\n";

	output << "\n## 5. Financial\n\n";
	output << "### Revenue growth по периодам\n\n";

	for (const DueDiligenceMetric& metric : revenueSeries)
	{
		output << FormatMetricLine(metric) << "\n";
	}

	output << "\n"
		   << FormatMetricLine(cagr) << "\n\n";
	output << "### Метрики последнего отчётного периода\n\n";

	for (const DueDiligenceMetric& metric : latestMetrics)
	{
		output << FormatMetricLine(metric) << "\n";
	}

	output << "\n### Флаги\n\n";

	for (const DueDiligenceFlag& flag : flags)
	{
		if (flag.dimension == "FINANCIAL")
		{
			output << "- " << SeverityIcon(flag.severity) << " **" << flag.code << "**: " << flag.explanation << "\n";
		}
	}

	output << "\n## 6. Governance\n\n";

	for (const DueDiligenceFlag& flag : flags)
	{
		if (flag.dimension == "GOVERNANCE")
		{
			output << "- " << SeverityIcon(flag.severity) << " **" << flag.code << "**: " << flag.explanation << "\n";
		}
	}

	output << "\n"
		   << (aiAvailable ? aiResult.value("governance_assessment", std::string("н/д")) : "AI недоступен, оценка не сгенерирована.") << "\n";

	output << "\n## 7. Litigation\n\n";
	output << "Источник (Картотека арбитражных дел) не подключён. " << (aiAvailable ? aiResult.value("bankruptcy_enforcement_assessment", std::string()) : "") << "\n";

	output << "\n## 8. Bankruptcy and Enforcement\n\n";
	output << (aiAvailable ? aiResult.value("bankruptcy_enforcement_assessment", std::string("н/д")) : "AI недоступен, оценка не сгенерирована.") << "\n";

	output << "\n## 9. Business Consistency\n\n";
	output << (aiAvailable ? aiResult.value("business_consistency_assessment", std::string("н/д")) : "AI недоступен, оценка не сгенерирована.") << "\n";

	output << "\n## 10. Event Timeline\n\n";

	std::vector<BfoPeriodReport> sortedHistory = m_history;

	std::sort(
		sortedHistory.begin(),
		sortedHistory.end(),
		[](const BfoPeriodReport& lhs, const BfoPeriodReport& rhs) {
			return lhs.period < rhs.period;
		});

	for (const BfoPeriodReport& report : sortedHistory)
	{
		output << "- `" << report.period << "` FINANCIAL_PERIOD — сдано " << report.actualBfoDate.value_or("н/д")
			   << ", выручка " << (report.gainSum ? std::to_string(*report.gainSum) : "н/д") << "\n";
	}

	output << "\n## 11. Data Quality\n\n";
	output << "- Финансовые данные нормализованы к RUB, единицы — рубли (как в исходном ответе bo.nalog.gov.ru).\n";
	output << "- Проверка бухгалтерского равенства (актив ≈ пассив): "
		   << (std::any_of(flags.begin(), flags.end(), [](const DueDiligenceFlag& f) { return f.code == "DATA_INCONSISTENCY"; }) ? "НЕСООТВЕТСТВИЕ" : "сходится") << "\n";

	output << "\n## 12. Source Status\n\n";
	output << "- `bo.nalog.gov.ru /nbo/organizations/{id}` — SUCCESS\n";
	output << "- `bo.nalog.gov.ru /nbo/organizations/{id}/bfo/` — SUCCESS\n";
	output << "- `bo.nalog.gov.ru /nbo/details/{type}` — " << (m_latestPeriodDetails.empty() ? "не запрашивался" : "SUCCESS") << "\n";
	output << "- `Polza.ai /chat/completions` (" << aiModel << ") — " << (aiAvailable ? "SUCCESS" : "UNAVAILABLE") << "\n";
	output << "- ЕГРЮЛ/ЕГРИП, КАД, ФССП, Федресурс, закупки, веб — UNAVAILABLE (источник не подключён)\n";

	output << "\n## 13. Required Next Actions\n\n";
	output << "- Проверить компанию вручную в Картотеке арбитражных дел (kad.arbitr.ru) по ИНН.\n";
	output << "- Проверить исполнительные производства в банке данных ФССП.\n";
	output << "- Проверить сообщения о банкротстве на Федресурсе.\n";

	if (aiAvailable && aiResult.contains("next_actions") && aiResult["next_actions"].is_array())
	{
		for (const auto& action : aiResult["next_actions"])
		{
			if (action.is_string())
			{
				output << "- " << action.get<std::string>() << " _(предложено ИИ)_\n";
			}
		}
	}

	output << "\n## 14. Methodology Limitations\n\n";
	output << "Отчёт построен только на одном публичном источнике — bo.nalog.gov.ru (бухгалтерская "
		   << "(финансовая) отчётность). Разделы Legal/Governance(частично)/Litigation/Bankruptcy/"
		   << "Enforcement/Business Consistency не имеют реального источника данных в этой системе и "
		   << "заполнены эвристической оценкой ИИ (" << aiModel << "). Раздел Financial построен на "
		   << "реальных цифрах официальной отчётности.\n";

	return output.str();
}

std::string DueDiligenceAnalyzer::BuildMarkdownReport() const
{
	const BfoPeriodReport* latestPeriod = FindLatestPeriodWithData();

	if (latestPeriod == nullptr)
	{
		return "# Public Due Diligence — " + m_profile.fullName.value_or(m_profile.shortName)
			+ "\n\nНет доступных периодов БФО с детализацией отчётности.\n";
	}

	const BfoCorrection* correction = FindPrimaryCorrection(*latestPeriod);

	const std::vector<DueDiligenceMetric> revenueSeries = ComputeRevenueGrowthSeries();
	const DueDiligenceMetric cagr = ComputeCagr();

	std::vector<DueDiligenceMetric> latestMetrics;
	std::vector<DueDiligenceFlag> flags;

	if (correction != nullptr)
	{
		latestMetrics = ComputeLatestPeriodMetrics(*correction);
		flags = DetectFlags(*latestPeriod, *correction);
	}

	const nlohmann::json context = BuildAiContext(latestMetrics, flags);
	const nlohmann::json aiResult = RequestAiEnrichment(context);

	return RenderMarkdown(revenueSeries, cagr, latestMetrics, flags, aiResult);
}
