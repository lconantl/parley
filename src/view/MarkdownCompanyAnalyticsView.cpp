#include "MarkdownCompanyAnalyticsView.hpp"

#include "common/output/md/MarkdownBuilder.hpp"
#include "finance/Metric.hpp"
#include <utility>

namespace
{
constexpr int TitleLevel = 1;
constexpr int SectionLevel = 2;
constexpr int GroupLevel = 3;

std::string DescribeStatus(const AnalyticsStatus status)
{
	switch (status)
	{
	case AnalyticsStatus::NoData:
		return "данных о компании нет";
	case AnalyticsStatus::EstimatedOnly:
		return "только оценки, отчетность недоступна";
	case AnalyticsStatus::Partial:
		return "отчетность частичная, пробелы закрыты оценками";
	case AnalyticsStatus::Complete:
		return "все показатели определены";
	}

	return "статус неизвестен";
}

std::string PickTitle(const CompanyAnalytics& analytics)
{
	if (!analytics.name.empty())
	{
		return analytics.name;
	}

	if (!analytics.identifier.empty())
	{
		return "Организация " + analytics.identifier;
	}

	return "Организация";
}

void AddRequisite(
	std::vector<std::vector<std::string>>& rows,
	const std::string& title,
	const std::string& value)
{
	if (value.empty())
	{
		return;
	}

	rows.push_back({title, value});
}
} // namespace

MarkdownCompanyAnalyticsView::MarkdownCompanyAnalyticsView(MetricFormatOptions options)
	: m_formatter(std::move(options))
{
}

std::string MarkdownCompanyAnalyticsView::Render(const CompanyAnalytics& analytics) const
{
	const MetricReport report(analytics);

	MarkdownBuilder builder;

	RenderTitle(builder, analytics);
	RenderSummary(builder, analytics);
	RenderMetricGroups(builder, report);
	RenderSeries(builder, analytics);
	RenderSegments(builder, analytics);
	RenderCompetitors(builder, analytics);
	RenderOperations(builder, analytics);
	RenderRisks(builder, analytics);
	RenderRelatedParties(builder, analytics);
	RenderAssumptions(builder, analytics);

	return builder.Build();
}

void MarkdownCompanyAnalyticsView::RenderTitle(
	MarkdownBuilder& builder,
	const CompanyAnalytics& analytics) const
{
	builder.AddHeader(PickTitle(analytics), TitleLevel);

	if (!analytics.activity.empty())
	{
		builder.AddQuote(analytics.activity);
	}

	builder.AddParagraph("Отчетный год: " + std::to_string(analytics.year)
		+ ". Полнота данных: " + DescribeStatus(analytics.status) + ".");
}

void MarkdownCompanyAnalyticsView::RenderSummary(
	MarkdownBuilder& builder,
	const CompanyAnalytics& analytics) const
{
	std::vector<std::vector<std::string>> rows;

	AddRequisite(rows, "ИНН", analytics.identifier);
	AddRequisite(rows, "Организационно-правовая форма", analytics.legalForm);
	AddRequisite(rows, "Адрес", analytics.region);
	AddRequisite(rows, "Рынок", analytics.market.definition);

	if (rows.empty())
	{
		return;
	}

	builder.AddHeader("Реквизиты", SectionLevel);
	builder.AddTable({"Поле", "Значение"}, rows);
}

std::vector<std::string> MarkdownCompanyAnalyticsView::BuildMetricHeaders() const
{
	std::vector<std::string> headers{"Показатель", "Значение"};

	if (m_formatter.ShowsOrigin())
	{
		headers.push_back("Источник");
	}

	if (m_formatter.ShowsConfidence())
	{
		headers.push_back("Уверенность");
	}

	return headers;
}

std::vector<std::string> MarkdownCompanyAnalyticsView::BuildMetricCells(const MetricRow& row) const
{
	std::vector<std::string> cells{row.title, m_formatter.FormatValue(row.value, row.unit)};

	if (m_formatter.ShowsOrigin())
	{
		cells.push_back(m_formatter.FormatOrigin(row.value));
	}

	if (m_formatter.ShowsConfidence())
	{
		cells.push_back(m_formatter.FormatConfidence(row.value));
	}

	return cells;
}

void MarkdownCompanyAnalyticsView::RenderMetricGroups(
	MarkdownBuilder& builder,
	const MetricReport& report) const
{
	builder.AddHeader("Показатели", SectionLevel);

	for (const MetricGroup group : MetricReport::ListGroups())
	{
		std::vector<std::vector<std::string>> rows;

		for (const auto& row : report.GetGroup(group))
		{
			if (!m_formatter.ShouldShow(row.value))
			{
				continue;
			}

			rows.push_back(BuildMetricCells(row));
		}

		if (rows.empty())
		{
			continue;
		}

		builder.AddHeader(MetricReport::DescribeGroup(group), GroupLevel);
		builder.AddTable(BuildMetricHeaders(), rows);
	}
}

void MarkdownCompanyAnalyticsView::RenderSeries(
	MarkdownBuilder& builder,
	const CompanyAnalytics& analytics) const
{
	if (analytics.series.empty())
	{
		return;
	}

	std::vector<std::vector<std::string>> rows;

	for (const auto& snapshot : analytics.series)
	{
		rows.push_back({std::to_string(snapshot.year),
			m_formatter.FormatValue(snapshot.revenue, MetricUnit::Money),
			m_formatter.FormatValue(snapshot.revenueGrowth, MetricUnit::Percent),
			m_formatter.FormatValue(snapshot.ebitda, MetricUnit::Money),
			m_formatter.FormatValue(snapshot.netProfit, MetricUnit::Money),
			m_formatter.FormatValue(snapshot.freeCashFlow, MetricUnit::Money),
			m_formatter.FormatValue(snapshot.totalDebt, MetricUnit::Money)});
	}

	builder.AddHeader("Динамика по годам", SectionLevel);
	builder.AddTable(
		{"Год", "Выручка", "Рост", "EBITDA", "Чистая прибыль", "FCF", "Долг"},
		rows);
}

void MarkdownCompanyAnalyticsView::RenderSegments(
	MarkdownBuilder& builder,
	const CompanyAnalytics& analytics) const
{
	if (analytics.segments.empty())
	{
		return;
	}

	std::vector<std::vector<std::string>> rows;

	for (const auto& segment : analytics.segments)
	{
		rows.push_back({segment.name,
			m_formatter.FormatValue(segment.revenue, MetricUnit::Money),
			m_formatter.FormatValue(segment.revenueShare, MetricUnit::Percent),
			m_formatter.FormatValue(segment.profit, MetricUnit::Money),
			m_formatter.FormatValue(segment.growth, MetricUnit::Percent)});
	}

	builder.AddHeader("Сегменты бизнеса", SectionLevel);
	builder.AddTable({"Сегмент", "Выручка", "Доля", "Прибыль", "Рост"}, rows);
}

void MarkdownCompanyAnalyticsView::RenderCompetitors(
	MarkdownBuilder& builder,
	const CompanyAnalytics& analytics) const
{
	if (analytics.competitors.empty())
	{
		return;
	}

	std::vector<std::vector<std::string>> rows;

	for (const auto& competitor : analytics.competitors)
	{
		rows.push_back({competitor.name,
			competitor.inn,
			m_formatter.FormatValue(competitor.revenue, MetricUnit::Money),
			m_formatter.FormatValue(competitor.revenueGrowth, MetricUnit::Percent),
			m_formatter.FormatValue(competitor.margin, MetricUnit::Percent),
			m_formatter.FormatValue(competitor.valuation, MetricUnit::Money)});
	}

	builder.AddHeader("Конкуренты", SectionLevel);
	builder.AddTable({"Компания", "ИНН", "Выручка", "Рост", "Маржа", "Оценка"}, rows);
}

void MarkdownCompanyAnalyticsView::RenderOperations(
	MarkdownBuilder& builder,
	const CompanyAnalytics& analytics) const
{
	if (analytics.operations.empty())
	{
		return;
	}

	std::vector<std::vector<std::string>> rows;

	for (const auto& indicator : analytics.operations)
	{
		rows.push_back({indicator.name,
			m_formatter.FormatValue(indicator.value, MetricUnit::Text),
			indicator.unit});
	}

	builder.AddHeader("Операционные показатели", SectionLevel);
	builder.AddTable({"Показатель", "Значение", "Единица"}, rows);
}

void MarkdownCompanyAnalyticsView::RenderRisks(
	MarkdownBuilder& builder,
	const CompanyAnalytics& analytics) const
{
	if (analytics.legalRisks.empty())
	{
		return;
	}

	std::vector<std::string> items;

	for (const auto& risk : analytics.legalRisks)
	{
		std::string item = risk.title;

		if (!risk.description.empty())
		{
			item += ". " + risk.description;
		}

		if (!risk.source.empty())
		{
			item += " (" + risk.source + ")";
		}

		items.push_back(item);
	}

	builder.AddHeader("Юридические и регуляторные риски", SectionLevel);
	builder.AddOrderedList(items);
}

void MarkdownCompanyAnalyticsView::RenderRelatedParties(
	MarkdownBuilder& builder,
	const CompanyAnalytics& analytics) const
{
	if (analytics.relatedParties.empty())
	{
		return;
	}

	std::vector<std::vector<std::string>> rows;

	for (const auto& party : analytics.relatedParties)
	{
		rows.push_back({party.name, party.role, party.inn, party.share});
	}

	builder.AddHeader("Владельцы, руководство и связанные компании", SectionLevel);
	builder.AddTable({"Лицо или компания", "Роль", "ИНН", "Доля"}, rows);
}

void MarkdownCompanyAnalyticsView::RenderAssumptions(
	MarkdownBuilder& builder,
	const CompanyAnalytics& analytics) const
{
	if (analytics.assumptions.empty())
	{
		return;
	}

	builder.AddHeader("Допущения", SectionLevel);
	builder.AddOrderedList(analytics.assumptions);
}
