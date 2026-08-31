#include "Company.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
void AssertIsMethodNameValid(const std::string& method)
{
	if (method.empty())
	{
		throw std::invalid_argument("Название метода не может быть пустым");
	}
}

void AssertIsMethodExists(
	const std::unordered_map<std::string, nlohmann::json>& data,
	const std::string& method)
{
	if (!data.contains(method))
	{
		throw std::out_of_range("Данные указанного метода отсутствуют");
	}
}

void AssertIsProfileLoaded(const std::optional<CompanyProfile>& profile)
{
	if (!profile.has_value())
	{
		throw std::out_of_range("Профиль организации не загружен");
	}
}

void AssertIsAnalyticsLoaded(const std::optional<CompanyAnalytics>& analytics)
{
	if (!analytics.has_value())
	{
		throw std::out_of_range("Аналитика по организации не рассчитана");
	}
}
} // namespace

void AssertIsNotEmpty(const std::string& id)
{
	if (id.empty())
	{
		throw std::invalid_argument("Идентификатор организации не может быть пустым");
	}
}

Company::Company(std::string id)
	: m_id(std::move(id))
{
	AssertIsNotEmpty(m_id);
}

void Company::SetData(
	const std::string& method,
	nlohmann::json data)
{
	AssertIsMethodNameValid(method);
	std::lock_guard lock(m_mutex);
	m_data[method] = std::move(data);
}

bool Company::HasData(const std::string& method) const
{
	AssertIsMethodNameValid(method);
	std::lock_guard lock(m_mutex);
	return m_data.contains(method);
}

const nlohmann::json& Company::GetData(const std::string& method) const
{
	AssertIsMethodNameValid(method);
	std::lock_guard lock(m_mutex);
	AssertIsMethodExists(m_data, method);
	return m_data.at(method);
}

bool Company::HasProfile() const
{
	std::lock_guard lock(m_mutex);
	return m_profile.has_value();
}

void Company::SetProfile(CompanyProfile profile)
{
	std::lock_guard lock(m_mutex);
	m_profile = std::move(profile);
}

CompanyProfile Company::GetProfile() const
{
	std::lock_guard lock(m_mutex);
	AssertIsProfileLoaded(m_profile);
	return m_profile.value();
}

bool Company::HasAnalytics() const
{
	std::lock_guard lock(m_mutex);
	return m_analytics.has_value();
}

void Company::SetAnalytics(CompanyAnalytics analytics)
{
	std::lock_guard lock(m_mutex);
	m_analytics = std::move(analytics);
}

CompanyAnalytics Company::GetAnalytics() const
{
	std::lock_guard lock(m_mutex);
	AssertIsAnalyticsLoaded(m_analytics);
	return m_analytics.value();
}

const std::string& Company::GetIdentifier() const noexcept
{
	return m_id;
}

std::size_t Company::GetMethodCount() const noexcept
{
	std::lock_guard lock(m_mutex);
	return m_data.size();
}

void Company::PrintJson() const
{
	std::lock_guard lock(m_mutex);

	for (const auto& [method, data] : m_data)
	{
		std::cout << "========== " << method << " ==========" << std::endl;
		std::cout << data.dump(5) << std::endl;
	}
}