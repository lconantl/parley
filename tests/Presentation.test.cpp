#include "TestEnvironment.hpp"
#include "system/InstanceLock.hpp"
#include "ui/Keyboard.hpp"
#include "ui/Presenter.hpp"
#include "ui/Text.hpp"
#include <fstream>
#include <gtest/gtest.h>

using namespace testing;

TEST(TextTest, PreservesUnicode)
{
	const std::string text = "Привет 👨‍💻 — <актив> & покупатель\n";
	std::string source;
	for (int index = 0; index < 400; ++index)
	{
		source += text;
	}
	std::string restored;
	for (const auto& part : Text::Split(source))
	{
		EXPECT_LE(Text::Length(part), 3500);
		EXPECT_TRUE(nlohmann::json::accept(nlohmann::json(part).dump()));
		restored += part;
	}
	EXPECT_EQ(restored, source);
	EXPECT_EQ(Text::Length("😀я"), 3);
}

TEST(TextTest, EscapesHtml)
{
	EXPECT_EQ(Text::Escape("<&>\""), "&lt;&amp;&gt;&quot;");
	EXPECT_EQ(Text::Shorten("Привет😀", 5), "Прив…");
}

TEST(PresenterTest, FitsAllCards)
{
	TestEnvironment env;
	for (int index = 1; index <= 100; ++index)
	{
		const auto match = *env.m_store->Next(111);
		const auto progress = env.m_store->GetProgress(111);
		EXPECT_LT(Text::Length(Presenter::Card(match, progress, "")), 4000);
		const auto keyboard = Keyboard::Card(match, "", env.m_store->LastDecision(111));
		for (const auto& row : keyboard["inline_keyboard"])
		{
			for (const auto& button : row)
			{
				EXPECT_LE(button["callback_data"].get<std::string>().size(), 64);
			}
		}
		env.m_store->Decide(111, match.id, "like", index);
	}
}

TEST(ConfigTest, ReadsSpacesAndQuotes)
{
	TestEnvironment env;
	const auto path = env.m_path / "extra.env";
	{
		std::ofstream file(path);
		file << "\xEF\xBB\xBF"
				"export BOT_TOKEN='123456:TEST_ONLY_NOT_A_REAL_BOT_TOKEN'\r\n"
				"ALLOWED_USERS=111, 222 # список\r\nMATCHES_DB=\"data/my matches.sqlite3\"\r\n"
				"POLZA_API_KEY=unused\r\n";
	}
	const auto config = Config::Load(path);
	EXPECT_TRUE(config.IsAllowed(111));
	EXPECT_TRUE(config.IsAllowed(222));
	EXPECT_FALSE(config.IsAllowed(333));
	EXPECT_EQ(config.GetDatabasePath(), env.m_path / "data/my matches.sqlite3");
}

TEST(ConfigTest, RejectsEmptyAccessList)
{
	TestEnvironment env;
	const auto path = env.m_path / "bad.env";
	{
		std::ofstream file(path);
		file << "BOT_TOKEN=123456:TEST_ONLY_NOT_A_REAL_BOT_TOKEN\nALLOWED_USERS=\n";
	}
	EXPECT_THROW(Config::Load(path), std::runtime_error);
}

TEST(InstanceLockTest, RejectsSecondInstance)
{
	TestEnvironment env;
	const InstanceLock first(env.m_path / "matches.sqlite3");
	EXPECT_THROW(InstanceLock second(env.m_path / "matches.sqlite3"), std::runtime_error);
}
