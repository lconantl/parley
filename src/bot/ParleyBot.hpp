#pragma once

#include "AccessPolicy.hpp"
#include "CommandRouter.hpp"
#include "SessionRegistry.hpp"
#include "ai/DueDiligenceNarrator.hpp"
#include "common/config/Config.hpp"
#include "common/output/DueDiligenceDeckBuilder.hpp"
#include "common/output/pdf/theme/Theme.hpp"
#include "common/pool/WorkerPool.hpp"
#include "finance/MetricFormatter.hpp"
#include "viewmodel/CompanyAnalyticsViewModel.hpp"

#include <filesystem>
#include <memory>

namespace TgBot
{
class Bot;
class Message;
} // namespace TgBot

class ParleyBot
{
public:
	struct Dependencies
	{
		std::shared_ptr<CompanyAnalyticsViewModel> analyticsViewModel;
		std::shared_ptr<DueDiligenceNarrator> narrator;
		Theme theme;
		MetricFormatOptions formatOptions;
		DueDiligenceOptions reportOptions;
		std::filesystem::path outputDirectory;
	};

	ParleyBot(const Config& config, Dependencies dependencies);
	~ParleyBot();

	ParleyBot(const ParleyBot&) = delete;
	ParleyBot& operator=(const ParleyBot&) = delete;

	void Run() const;
	void Stop() const;

private:
	void RegisterCommands(Dependencies dependencies);
	void SubscribeToMessages();
	void PublishCommandMenu() const;

	void HandleMessage(const std::shared_ptr<TgBot::Message>& rawMessage) const;
	void HandleSession(const ParsedMessage& message) const;
	void ScheduleSession(const ParsedMessage& message) const;

	std::unique_ptr<TgBot::Bot> m_bot;
	AccessPolicy m_accessPolicy;
	CommandRouter m_router;
	mutable SessionRegistry m_sessions;
	mutable WorkerPool m_workers;
};