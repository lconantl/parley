#include "Company.hpp"

#include <fstream>
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

void AssertIsNotEmpty(const std::string& id)
{
	if (id.empty())
	{
		throw std::invalid_argument("Идентификатор организации не может быть пустым");
	}
}

void AssertIsFileOpen(const std::ofstream& file, const std::filesystem::path& path)
{
	if (!file.is_open())
	{
		throw std::runtime_error("Не удалось открыть файл для записи: " + path.string());
	}
}

void AssertIsMetricsPresent(const bool hasMetrics)
{
	if (!hasMetrics)
	{
		throw std::logic_error("Метрики компании не установлены");
	}
}
} // namespace

Company::Company(std::string id)
	: m_id(std::move(id))
	, m_metrics()
{
	AssertIsNotEmpty(m_id);
}

void Company::SetData(const std::string& method, nlohmann::json data)
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

bool Company::HasMetrics() const
{
	std::lock_guard lock(m_mutex);
	return m_hasMetrics;
}

void Company::SetMetrics(CompanyMetrics metrics)
{
	std::lock_guard lock(m_mutex);
	m_metrics = std::move(metrics);
	m_hasMetrics = true;
}

const CompanyMetrics& Company::GetMetrics() const
{
	std::lock_guard lock(m_mutex);
	AssertIsMetricsPresent(m_hasMetrics);
	return m_metrics;
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

void Company::SaveToJson(const std::filesystem::path& path) const
{
	std::lock_guard lock(m_mutex);
	nlohmann::json outputData;

	for (const auto& [method, data] : m_data)
	{
		outputData[method] = data;
	}

	std::ofstream file(path);
	AssertIsFileOpen(file, path);

	file << outputData.dump(4) << std::endl;
}