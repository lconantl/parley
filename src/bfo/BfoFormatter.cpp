#include "BfoFormatter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>

namespace
{

constexpr std::size_t MAX_MESSAGE_LENGTH = 3800;

std::string JoinInts(
    const std::vector<int>& values
)
{
    std::ostringstream output;

    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index != 0)
        {
            output << ", ";
        }

        output << values[index];
    }

    return output.str();
}

std::string BoolToRu(
    const bool value
)
{
    return value ? "да" : "нет";
}

} // namespace

std::vector<std::string> BfoFormatter::Format(
    const SearchResponse& response
)
{
    if (response.companies.empty())
    {
        return {"Ничего не найдено."};
    }

    std::vector<std::string> messages;
    std::string current;

    for (const auto& company : response.companies)
    {
        const std::string card = FormatCompany(company);

        if (
            !current.empty()
            && current.size() + card.size() + 1 > MAX_MESSAGE_LENGTH
        )
        {
            messages.push_back(current);
            current.clear();
        }

        if (!current.empty())
        {
            current += "\n";
        }

        current += card;
    }

    const std::string footer =
        "\nНайдено всего: "
        + std::to_string(response.totalElements);

    if (current.size() + footer.size() > MAX_MESSAGE_LENGTH)
    {
        messages.push_back(current);
        current = footer.substr(1);
    }
    else
    {
        current += footer;
    }

    if (!current.empty())
    {
        messages.push_back(current);
    }

    return messages;
}

std::string BfoFormatter::FormatCompany(
    const CompanySearchResult& company
)
{
    std::ostringstream output;

    output
        << "🏢 "
        << EscapeHtml(company.shortName)
        << "\n\n";

    output
        << "ID: " << company.id << "\n"
        << "ИНН: " << EscapeHtml(company.inn) << "\n"
        << "ОГРН: " << EscapeHtml(company.ogrn) << "\n\n";

    output
        << "Статус: " << OrDash(company.statusCode) << "\n"
        << "Дата статуса: " << OrDash(company.statusDate) << "\n\n";

    output
        << "ОКВЭД: " << EscapeHtml(company.okved2) << "\n"
        << "ОКОПФ: " << company.okopf << "\n"
        << "ОКАТО: " << OrDash(company.okato) << "\n"
        << "ОКПО: " << OrDash(company.okpo) << "\n"
        << "ОКФС: " << OrDash(company.okfs) << "\n\n";

    output << "Адрес:\n";

    if (company.index)
    {
        output << EscapeHtml(*company.index) << "\n";
    }

    if (company.region)
    {
        output << EscapeHtml(*company.region) << "\n";
    }

    if (company.district)
    {
        output << EscapeHtml(*company.district) << "\n";
    }

    if (company.city)
    {
        output << EscapeHtml(*company.city) << "\n";
    }

    if (company.settlement)
    {
        output << EscapeHtml(*company.settlement) << "\n";
    }

    if (company.street && company.house)
    {
        output << EscapeHtml(*company.street) << ", " << EscapeHtml(*company.house) << "\n";
    }
    else if (company.street)
    {
        output << EscapeHtml(*company.street) << "\n";
    }
    else if (company.house)
    {
        output << EscapeHtml(*company.house) << "\n";
    }

    if (company.building)
    {
        output << "корп. " << EscapeHtml(*company.building) << "\n";
    }

    if (company.office)
    {
        output << "офис " << EscapeHtml(*company.office) << "\n";
    }

    output << "\n";

    output << "БФО:\n";
    output << "Период: " << OrDash(company.bfo.period) << "\n";
    output << "Дата БФО: " << OrDash(company.bfo.actualBfoDate) << "\n";
    output << "Сумма: " << FormatGainSum(company.bfo.gainSum) << "\n";
    output << "КНД: " << OrDash(company.bfo.knd) << "\n";
    output << "АЗ: " << BoolToRu(company.bfo.hasAz) << "\n";
    output << "КС: " << BoolToRu(company.bfo.hasKs) << "\n";
    output << "Коррекция: " << company.bfo.actualCorrectionNumber << "\n";
    output << "Дата коррекции: " << OrDash(company.bfo.actualCorrectionDate) << "\n";
    output << "ЦБ: " << BoolToRu(company.bfo.isCb) << "\n";
    output << "Типы периода: "
        << (company.bfo.bfoPeriodTypes.empty() ? "—" : JoinInts(company.bfo.bfoPeriodTypes))
        << "\n\n";

    output << "Карточка:\n" << EscapeHtml(company.cardUrl);

    return output.str();
}

std::string BfoFormatter::OrDash(
    const std::optional<std::string>& value
)
{
    if (!value || value->empty())
    {
        return "—";
    }

    return EscapeHtml(*value);
}

std::string BfoFormatter::FormatGainSum(
    const std::optional<double>& value
)
{
    if (!value)
    {
        return "—";
    }

    if (std::trunc(*value) == *value)
    {
        return std::to_string(static_cast<long long>(*value));
    }

    std::ostringstream output;

    output.precision(2);
    output << std::fixed << *value;

    return output.str();
}

std::string BfoFormatter::EscapeHtml(
    const std::string& value
)
{
    std::string result;

    result.reserve(
        value.size()
    );

    for (
        const char character :
        value
    )
    {
        switch (character)
        {
        case '&':
            result += "&amp;";
            break;

        case '<':
            result += "&lt;";
            break;

        case '>':
            result += "&gt;";
            break;

        case '"':
            result += "&quot;";
            break;

        default:
            result += character;
            break;
        }
    }

    return result;
}
