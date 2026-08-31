#pragma once

#include "bot/ICommandHandler.hpp"
#include "finance/CompanyAnalytics.hpp"
#include "viewmodel/CompanyAnalyticsViewModel.hpp"
#include <filesystem>
#include <memory>
#include <string>

class AnalyticsCommandHandler : public ICommandHandler
{
public:
	AnalyticsCommandHandler(
		std::shared_ptr<CompanyAnalyticsViewModel> viewModel,
		std::filesystem::path outputDirectory);

	void Execute(const CommandContext& context) final;

protected:
	virtual std::filesystem::path BuildDocument(const CompanyAnalytics& analytics) const = 0;
	virtual std::string BuildCaption(const CompanyAnalytics& analytics) const = 0;
	virtual std::string GetMimeType() const = 0;

	const std::filesystem::path& GetOutputDirectory() const noexcept;

private:
	std::string ExtractIdentifier(const ParsedMessage& message) const;

	std::shared_ptr<CompanyAnalyticsViewModel> m_viewModel;
	std::filesystem::path m_outputDirectory;
};