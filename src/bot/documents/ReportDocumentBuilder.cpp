#include "ReportDocumentBuilder.hpp"

#include <fstream>
#include <stdexcept>

namespace
{
constexpr auto Label = "Отчет";
constexpr auto FileExtension = ".md";
constexpr auto MimeType = "text/markdown";

void PrepareDirectory(const std::filesystem::path& directory)
{
	if (directory.empty() || std::filesystem::exists(directory))
	{
		return;
	}

	std::filesystem::create_directories(directory);
}

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

ReportDocumentBuilder::ReportDocumentBuilder(
	std::filesystem::path outputDirectory,
	MetricFormatOptions formatOptions)
	: m_outputDirectory(std::move(outputDirectory))
	, m_view(formatOptions)
	, m_formatter(formatOptions)
{
	PrepareDirectory(m_outputDirectory);
}

std::string ReportDocumentBuilder::GetLabel() const
{
	return Label;
}

DocumentBuildResult ReportDocumentBuilder::Build(
	const CompanyAnalytics& analytics,
	const DocumentBuildOptions& /*options*/) const
{
	const std::filesystem::path path = m_outputDirectory / BuildFileName(analytics);

	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	AssertIsFileOpen(file, path);

	file << m_view.Render(analytics);

	std::string caption = analytics.name.empty() ? analytics.identifier : analytics.name;
	caption += ", " + std::to_string(analytics.year) + "\n";
	caption += "Выручка: " + m_formatter.FormatValue(analytics.revenue.revenue, MetricUnit::Money) + "\n";
	caption += "EBITDA: " + m_formatter.FormatValue(analytics.profit.ebitda, MetricUnit::Money) + "\n";
	caption += "Чистая прибыль: " + m_formatter.FormatValue(analytics.profit.netProfit, MetricUnit::Money);

	return {path, caption, MimeType};
}
