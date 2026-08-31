#include "MarkdownReportCommandHandler.hpp"
#include <fstream>
#include <stdexcept>

namespace
{
constexpr auto CommandName = "report";
constexpr auto CommandDescription = "Получить отчет по ИНН";
constexpr auto FileExtension = ".md";
constexpr auto MimeType = "text/markdown";

void AssertIsFileOpen(const std::ofstream& file, const std::filesystem::path& path)
{
	if (!file.is_open())
	{
		throw std::runtime_error("Не удалось создать файл отчета: " + path.string());
	}
}

std::string BuildFileName(const CompanyAnalytics& analytics)
{
	const std::string identifier = analytics.identifier.empty() ? "company" : analytics.identifier;

	return identifier + "-" + std::to_string(analytics.year) + FileExtension;
}
} // namespace

MarkdownReportCommandHandler::MarkdownReportCommandHandler(
	std::shared_ptr<CompanyAnalyticsViewModel> viewModel,
	std::filesystem::path outputDirectory,
	MetricFormatOptions formatOptions)
	: AnalyticsCommandHandler(std::move(viewModel), std::move(outputDirectory))
	, m_view(formatOptions)
	, m_formatter(formatOptions)
{
}

std::string MarkdownReportCommandHandler::GetName() const
{
	return CommandName;
}

std::string MarkdownReportCommandHandler::GetDescription() const
{
	return CommandDescription;
}

std::string MarkdownReportCommandHandler::GetMimeType() const
{
	return MimeType;
}

std::filesystem::path MarkdownReportCommandHandler::BuildDocument(
	const CompanyAnalytics& analytics) const
{
	const std::filesystem::path path = GetOutputDirectory() / BuildFileName(analytics);

	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	AssertIsFileOpen(file, path);

	file << m_view.Render(analytics);

	return path;
}

std::string MarkdownReportCommandHandler::BuildCaption(const CompanyAnalytics& analytics) const
{
	std::string caption = analytics.name.empty() ? analytics.identifier : analytics.name;
	caption += ", " + std::to_string(analytics.year) + "\n";
	caption += "Выручка: "
		+ m_formatter.FormatValue(analytics.revenue.revenue, MetricUnit::Money) + "\n";
	caption += "EBITDA: " + m_formatter.FormatValue(analytics.profit.ebitda, MetricUnit::Money)
		+ "\n";
	caption += "Чистая прибыль: "
		+ m_formatter.FormatValue(analytics.profit.netProfit, MetricUnit::Money);

	return caption;
}