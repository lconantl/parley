#include "Config.hpp"

#include "EnvLoader.hpp"

#include <stdexcept>
#include <utility>

namespace
{

std::string GetRequired(
    const EnvLoader::EnvData& data,
    const std::string& key
)
{
    const auto iterator =
        data.find(
            key
        );

    if (iterator == data.end())
    {
        throw std::runtime_error(
            "Отсутствует переменная окружения: "
            +
            key
        );
    }

    return iterator->second;
}

long GetLong(
    const EnvLoader::EnvData& data,
    const std::string& key
)
{
    try
    {
        return std::stol(
            GetRequired(
                data,
                key
            )
        );
    }
    catch (...)
    {
        throw std::runtime_error(
            "Некорректное числовое значение: "
            +
            key
        );
    }
}

std::vector<std::string> ParseUsers(
    const std::string& raw
)
{
    std::vector<std::string> result;

    std::size_t start = 0;

    while (start < raw.size())
    {
        const auto comma =
            raw.find(
                ',',
                start
            );

        const auto length =
            comma == std::string::npos
                ? raw.size() - start
                : comma - start;

        if (length > 0)
        {
            result.emplace_back(
                raw.substr(
                    start,
                    length
                )
            );
        }

        if (comma == std::string::npos)
        {
            break;
        }

        start =
            comma + 1;
    }

    return result;
}

}

Config Config::Load(
    const std::filesystem::path& filePath
)
{
    const auto data =
        EnvLoader::Load(
            filePath
        );

    const auto botToken =
        GetRequired(
            data,
            "BOT_TOKEN"
        );

    if (
        botToken.find(':')
        ==
        std::string::npos
    )
    {
        throw std::runtime_error(
            "Неверный BOT_TOKEN"
        );
    }

    return Config(
        botToken,
        ParseUsers(
            GetRequired(
                data,
                "ALLOWED_USERS"
            )
        ),
        GetRequired(
            data,
            "BFO_BASE_URL"
        ),
        GetRequired(
            data,
            "BFO_SEARCH_PATH"
        ),
        GetLong(
            data,
            "BFO_PAGE_SIZE"
        ),
        GetLong(
            data,
            "BFO_CONNECT_TIMEOUT_SEC"
        ),
        GetLong(
            data,
            "BFO_READ_TIMEOUT_SEC"
        ),
        GetLong(
            data,
            "BFO_WRITE_TIMEOUT_SEC"
        ),
        GetRequired(
            data,
            "BFO_USER_AGENT"
        )
    );
}

Config::Config(
    std::string botToken,
    std::vector<std::string> allowedUsers,
    std::string bfoBaseUrl,
    std::string bfoSearchPath,
    const long bfoPageSize,
    const long bfoConnectTimeoutSec,
    const long bfoReadTimeoutSec,
    const long bfoWriteTimeoutSec,
    std::string bfoUserAgent
)
    : m_botToken(std::move(botToken))
    , m_allowedUsers(std::move(allowedUsers))
    , m_bfoBaseUrl(std::move(bfoBaseUrl))
    , m_bfoSearchPath(std::move(bfoSearchPath))
    , m_bfoPageSize(bfoPageSize)
    , m_bfoConnectTimeoutSec(bfoConnectTimeoutSec)
    , m_bfoReadTimeoutSec(bfoReadTimeoutSec)
    , m_bfoWriteTimeoutSec(bfoWriteTimeoutSec)
    , m_bfoUserAgent(std::move(bfoUserAgent))
{
}

const std::string& Config::GetBotToken() const
{
    return m_botToken;
}

const std::vector<std::string>& Config::GetAllowedUsers() const
{
    return m_allowedUsers;
}

const std::string& Config::GetBfoBaseUrl() const
{
    return m_bfoBaseUrl;
}

const std::string& Config::GetBfoSearchPath() const
{
    return m_bfoSearchPath;
}

long Config::GetBfoPageSize() const
{
    return m_bfoPageSize;
}

long Config::GetBfoConnectTimeoutSec() const
{
    return m_bfoConnectTimeoutSec;
}

long Config::GetBfoReadTimeoutSec() const
{
    return m_bfoReadTimeoutSec;
}

long Config::GetBfoWriteTimeoutSec() const
{
    return m_bfoWriteTimeoutSec;
}

const std::string& Config::GetBfoUserAgent() const
{
    return m_bfoUserAgent;
}