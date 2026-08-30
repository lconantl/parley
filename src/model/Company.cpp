#include "Company.hpp"
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

void Company::SetData(const std::string& method, nlohmann::json data)
{
	AssertIsMethodNameValid(method);
	m_data[method] = std::move(data);
}

bool Company::HasData(const std::string& method) const
{
	AssertIsMethodNameValid(method);

	return m_data.contains(method);
}

const nlohmann::json& Company::GetData(const std::string& method) const
{
	AssertIsMethodNameValid(method);
	AssertIsMethodExists(m_data, method);

	return m_data.at(method);
}

const std::string& Company::GetIdentifier() const noexcept
{
	return m_id;
}

std::size_t Company::GetMethodCount() const noexcept
{
	return m_data.size();
}