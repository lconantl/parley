#include "AnalyticsCommandHandler.hpp"
#include "common/inn/Inn.hpp"
#include <stdexcept>
#include <utility>

namespace
{
constexpr auto StatusText = "Собираю данные и считаю показатели, это займет от 1 до 5 минут";
constexpr auto MissingIdentifierText = "Пришлите ИНН организации: 10 цифр для юридического лица или 12 для предпринимателя";
constexpr auto InvalidIdentifierText = "Это не похоже на ИНН. Проверьте контрольную сумму и пришлите номер еще раз";
constexpr auto NoDataText = "Данных об этой организации нет ни в реестрах, ни в открытых источниках";

void AssertIsViewModelValid(const std::shared_ptr<CompanyAnalyticsViewModel>& viewModel)
{
	if (viewModel == nullptr)
	{
		throw std::invalid_argument("Модель аналитики не может быть пустой");
	}
}

void PrepareDirectory(const std::filesystem::path& directory)
{
	if (directory.empty() || std::filesystem::exists(directory))
	{
		return;
	}

	std::filesystem::create_directories(directory);
}
} // namespace

AnalyticsCommandHandler::AnalyticsCommandHandler(
	std::shared_ptr<CompanyAnalyticsViewModel> viewModel,
	std::filesystem::path outputDirectory)
	: m_viewModel(std::move(viewModel))
	, m_outputDirectory(std::move(outputDirectory))
{
	AssertIsViewModelValid(m_viewModel);
	PrepareDirectory(m_outputDirectory);
}

const std::filesystem::path& AnalyticsCommandHandler::GetOutputDirectory() const noexcept
{
	return m_outputDirectory;
}

std::string AnalyticsCommandHandler::ExtractIdentifier(const ParsedMessage& message) const
{
	const std::string fromArguments = Inn::Extract(message.arguments);
	if (!fromArguments.empty())
	{
		return fromArguments;
	}

	return Inn::Extract(message.text);
}

void AnalyticsCommandHandler::Execute(const CommandContext& context)
{
	const std::string identifier = ExtractIdentifier(context.message);

	if (identifier.empty())
	{
		const bool hasDigits = !Inn::Normalize(context.message.text).empty();

		context.messages->SendText(
			context.message.chatId,
			hasDigits ? InvalidIdentifierText : MissingIdentifierText);

		return;
	}

	context.messages->SendStatus(context.message.chatId, StatusText);

	const CompanyAnalytics analytics = m_viewModel->Analyze(identifier);

	if (analytics.status == AnalyticsStatus::NoData)
	{
		context.messages->SendText(context.message.chatId, NoDataText);
		return;
	}

	const std::filesystem::path document = BuildDocument(analytics);
	context.messages->SendDocument(
		context.message.chatId, document, BuildCaption(analytics), GetMimeType());
}