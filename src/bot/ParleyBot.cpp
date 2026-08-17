#include "ParleyBot.hpp"

#include "bfo/BfoFormatter.hpp"

#include <algorithm>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

ParleyBot::ParleyBot(
    const Config& config
)
    : m_bot(
          config.GetBotToken()
      )
    , m_config(
          config
      )
    , m_scraper(
          config
      )
{
    SetupHandlers();
}

void ParleyBot::Run() const
{
    try
    {
        TgBot::TgLongPoll longPoll(
            m_bot
        );

        while (true)
        {
            longPoll.start();
        }
    }
    catch (
        const TgBot::TgException& exception
    )
    {
        throw std::runtime_error(
            std::string(
                "[Telegram] "
            )
            +
            exception.what()
        );
    }
}

void ParleyBot::SetupHandlers()
{
    m_bot.getEvents().onCommand(
        "start",
        [this](
            const TgBot::Message::Ptr& message
        )
        {
            if (
                message == nullptr
                ||
                !IsUserAllowed(
                    message->from->id
                )
            )
            {
                return;
            }

            m_bot.getApi().sendMessage(
                message->chat->id,
                "Отправь ИНН или название компании."
            );
        }
    );

    m_bot.getEvents().onAnyMessage(
        [this](
            const TgBot::Message::Ptr& message
        )
        {
            HandleMessage(
                message
            );
        }
    );
}

void ParleyBot::HandleMessage(
    const TgBot::Message::Ptr& message
) const
{
    if (
        message == nullptr
        ||
        message->from == nullptr
        ||
        message->chat == nullptr
    )
    {
        return;
    }

    if (
        !IsUserAllowed(
            message->from->id
        )
    )
    {
        return;
    }

    if (message->text.empty())
    {
        return;
    }

    if (
        IsCommand(
            message->text
        )
    )
    {
        return;
    }

    std::cout
        << "BFO search: "
        << message->text
        << std::endl;

    HandleBfoSearch(
        message->chat->id,
        message->text
    );
}

bool ParleyBot::IsUserAllowed(
    const int64_t userId
) const
{
    const auto userIdString =
        std::to_string(
            userId
        );

    const auto& users =
        m_config.GetAllowedUsers();

    return std::ranges::find(
        users,
        userIdString
    ) != users.end();
}

void ParleyBot::HandleBfoSearch(
    const int64_t chatId,
    const std::string& query
) const
{
    try
    {
        m_bot.getApi().sendMessage(
            chatId,
            "Ищу компанию на bo.nalog.gov.ru..."
        );

        const SearchResponse response =
            m_scraper.Search(
                query
            );

        const std::vector<std::string> messages =
            BfoFormatter::Format(
                response
            );

        for (const auto& message : messages)
        {
            m_bot.getApi().sendMessage(
                chatId,
                message,
                nullptr,
                nullptr,
                nullptr,
                "HTML"
            );
        }
    }
    catch (
        const std::exception& exception
    )
    {
        std::cerr
            << "[BFO] "
            << exception.what()
            << std::endl;

        m_bot.getApi().sendMessage(
            chatId,
            "Не удалось выполнить поиск в БФО. Попробуйте ещё раз позже."
        );
    }
}

bool ParleyBot::IsCommand(
    const std::string& text
)
{
    return !text.empty()
        && text.front() == '/';
}