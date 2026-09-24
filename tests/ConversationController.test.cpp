#include "bot/ConversationController.hpp"
#include "TestEnvironment.hpp"
#include "ui/Text.hpp"
#include <gtest/gtest.h>

using namespace testing;

TEST(ConversationControllerTest, ShowsStart)
{
	TestEnvironment env;
	ConversationController controller(env.m_config, *env.m_store, env.m_client);
	controller.Handle(MakeMessage(111, "/start"));
	ASSERT_EQ(env.m_client.m_calls.size(), 2);
	EXPECT_TRUE(env.m_client.m_calls[0].parameters["reply_markup"].contains("keyboard"));
}

TEST(ConversationControllerTest, BlocksUnknownUser)
{
	TestEnvironment env;
	ConversationController controller(env.m_config, *env.m_store, env.m_client);
	controller.Handle(MakeMessage(999, "/feed"));
	ASSERT_EQ(env.m_client.m_calls.size(), 1);
	EXPECT_NE(env.m_client.m_calls[0].parameters["text"].get<std::string>().find("Доступ закрыт"),
			  std::string::npos);
	EXPECT_EQ(env.m_store->GetProgress(999).liked, 0);
}

TEST(ConversationControllerTest, BlocksGroupAndForeignChat)
{
	TestEnvironment env;
	ConversationController controller(env.m_config, *env.m_store, env.m_client);
	auto message = MakeMessage(111, "/feed");
	message["message"]["chat"]["type"] = "supergroup";
	controller.Handle(message);
	EXPECT_TRUE(env.m_client.m_calls.empty());
	auto callback = MakeCallback(111, "like:" + env.m_store->Next(111)->id);
	callback["callback_query"]["message"]["chat"]["id"] = 222;
	controller.Handle(callback);
	EXPECT_EQ(env.m_store->GetProgress(111).liked, 0);
	EXPECT_EQ(env.m_client.m_calls.back().method, "answerCallbackQuery");
}

TEST(ConversationControllerTest, LikeSendsExactBody)
{
	TestEnvironment env;
	const auto match = *env.m_store->Next(111);
	const auto code = match.suggestedMode == "buy_side" ? "A1" : "B1";
	const auto expected = env.m_store->GetDraft(match.id, code).body;
	ConversationController controller(env.m_config, *env.m_store, env.m_client);
	controller.Handle(MakeCallback(111, "like:" + match.id));
	EXPECT_EQ(env.m_store->GetProgress(111).liked, 1);
	bool found = false;
	for (const auto& call : env.m_client.m_calls)
	{
		found |= call.parameters.value("text", "") == expected;
	}
	EXPECT_TRUE(found);
	EXPECT_EQ(env.m_client.m_calls.front().method, "answerCallbackQuery");
}

TEST(ConversationControllerTest, DoubleLikeDoesNotResend)
{
	TestEnvironment env;
	const auto match = *env.m_store->Next(111);
	ConversationController controller(env.m_config, *env.m_store, env.m_client);
	controller.Handle(MakeCallback(111, "like:" + match.id, 1));
	env.m_client.m_calls.clear();
	controller.Handle(MakeCallback(111, "like:" + match.id, 2));
	EXPECT_EQ(env.m_store->GetProgress(111).liked, 1);
	ASSERT_EQ(env.m_client.m_calls.size(), 2);
	EXPECT_EQ(env.m_client.m_calls.back().method, "editMessageText");
}

TEST(ConversationControllerTest, SkipAdvances)
{
	TestEnvironment env;
	ConversationController controller(env.m_config, *env.m_store, env.m_client);
	controller.Handle(MakeCallback(111, "no:" + env.m_store->Next(111)->id));
	EXPECT_EQ(env.m_store->Next(111)->rank, 2);
	EXPECT_NE(
		env.m_client.m_calls.back().parameters["text"].get<std::string>().find("Пара 2 из 100"),
		std::string::npos);
}

TEST(ConversationControllerTest, SendsAllFormats)
{
	TestEnvironment env;
	const auto match = *env.m_store->Next(111);
	ConversationController controller(env.m_config, *env.m_store, env.m_client);
	for (const auto* code : {"A1", "A2", "B1", "B2"})
	{
		env.m_client.m_calls.clear();
		controller.Handle(MakeCallback(111, std::string(code) + ":" + match.id));
		bool found = false;
		for (const auto& call : env.m_client.m_calls)
		{
			found |=
				call.parameters.value("text", "") == env.m_store->GetDraft(match.id, code).body;
		}
		EXPECT_TRUE(found) << code;
	}
}

TEST(ConversationControllerTest, ChangesMode)
{
	TestEnvironment env;
	const auto match = *env.m_store->Next(111);
	ConversationController controller(env.m_config, *env.m_store, env.m_client);
	controller.Handle(MakeCallback(111, "buy:" + match.id));
	EXPECT_EQ(env.m_store->GetMode(111, match), "buy_side");
	controller.Handle(MakeCallback(111, "sell:" + match.id));
	EXPECT_EQ(env.m_store->GetMode(111, match), "sell_side");
}

TEST(ConversationControllerTest, KeepsLikeOnSendFailure)
{
	TestEnvironment env;
	const auto match = *env.m_store->Next(111);
	ConversationController controller(env.m_config, *env.m_store, env.m_client);
	env.m_client.m_failOnSend = 1;
	EXPECT_THROW(controller.Handle(MakeCallback(111, "like:" + match.id)), std::runtime_error);
	EXPECT_EQ(env.m_store->Favorites(111, 0).front().id, match.id);
}

TEST(ConversationControllerTest, HandlesStaleButton)
{
	TestEnvironment env;
	ConversationController controller(env.m_config, *env.m_store, env.m_client);
	EXPECT_NO_THROW(controller.Handle(MakeCallback(111, "legacy:invalid")));
	EXPECT_EQ(env.m_store->GetProgress(111).liked, 0);
}

TEST(ConversationControllerTest, UndoCommandIsIdempotent)
{
	TestEnvironment env;
	ConversationController controller(env.m_config, *env.m_store, env.m_client);
	controller.Handle(MakeCallback(111, "no:" + env.m_store->Next(111)->id, 1));
	controller.Handle(MakeCallback(111, "no:" + env.m_store->Next(111)->id, 2));
	controller.Handle(MakeMessage(111, "/undo", 3));
	controller.Handle(MakeMessage(111, "/undo", 3));
	EXPECT_EQ(env.m_store->GetProgress(111).skipped, 1);
}
