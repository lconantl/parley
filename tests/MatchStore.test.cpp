#include "TestEnvironment.hpp"
#include "storage/Database.hpp"
#include <gtest/gtest.h>

using namespace testing;

TEST(MatchStoreTest, LoadsCatalog)
{
	TestEnvironment env;
	EXPECT_EQ(env.m_store->GetProgress(111).total, 100);
	EXPECT_EQ(env.m_store->Next(111)->rank, 1);
	EXPECT_EQ(env.m_store->Next(111)->score, 93);
}

TEST(MatchStoreTest, IsolatesUsers)
{
	TestEnvironment env;
	const auto first = *env.m_store->Next(111);
	EXPECT_TRUE(env.m_store->Decide(111, first.id, "like", 1));
	EXPECT_EQ(env.m_store->Next(111)->rank, 2);
	EXPECT_EQ(env.m_store->Next(222)->rank, 1);
	EXPECT_EQ(env.m_store->GetProgress(111).liked, 1);
	EXPECT_EQ(env.m_store->GetProgress(222).liked, 0);
}

TEST(MatchStoreTest, IgnoresDoubleClick)
{
	TestEnvironment env;
	const auto first = *env.m_store->Next(111);
	EXPECT_TRUE(env.m_store->Decide(111, first.id, "like", 1));
	EXPECT_FALSE(env.m_store->Decide(111, first.id, "dislike", 2));
	EXPECT_EQ(env.m_store->GetDecision(111, first.id), "like");
	EXPECT_EQ(env.m_store->GetProgress(111).liked, 1);
}

TEST(MatchStoreTest, RestoresDecision)
{
	TestEnvironment env;
	const auto first = *env.m_store->Next(111);
	env.m_store->Decide(111, first.id, "like", 1);
	env.m_store->Decide(111, first.id, "dislike", 2, true);
	const auto event = *env.m_store->LastDecision(111);
	EXPECT_EQ(env.m_store->Undo(111, event.id, 3), first.id);
	EXPECT_EQ(env.m_store->GetDecision(111, first.id), "like");
	EXPECT_FALSE(env.m_store->Undo(111, event.id, 4));
}

TEST(MatchStoreTest, UndoIsIdempotent)
{
	TestEnvironment env;
	env.m_store->Decide(111, env.m_store->Next(111)->id, "like", 1);
	env.m_store->Decide(111, env.m_store->Next(111)->id, "like", 2);
	EXPECT_TRUE(env.m_store->Undo(111, env.m_store->LastDecision(111)->id, 3));
	EXPECT_FALSE(env.m_store->Undo(111, env.m_store->LastDecision(111)->id, 3));
	EXPECT_EQ(env.m_store->GetProgress(111).liked, 1);
}

TEST(MatchStoreTest, RejectsStaleUndo)
{
	TestEnvironment env;
	env.m_store->Decide(111, env.m_store->Next(111)->id, "like", 1);
	const auto old = *env.m_store->LastDecision(111);
	env.m_store->Decide(111, env.m_store->Next(111)->id, "dislike", 2);
	EXPECT_FALSE(env.m_store->Undo(111, old.id, 3));
	EXPECT_FALSE(env.m_store->Undo(222, env.m_store->LastDecision(111)->id, 4));
	EXPECT_EQ(env.m_store->GetProgress(111).skipped, 1);
}

TEST(MatchStoreTest, DoesNotReplayUndoneLike)
{
	TestEnvironment env;
	const auto first = *env.m_store->Next(111);
	env.m_store->Decide(111, first.id, "like", 1);
	env.m_store->Undo(111, env.m_store->LastDecision(111)->id, 2);
	EXPECT_FALSE(env.m_store->Decide(111, first.id, "like", 1));
	EXPECT_TRUE(env.m_store->GetDecision(111, first.id).empty());
}

TEST(MatchStoreTest, SurvivesRestart)
{
	TestEnvironment env;
	const auto first = *env.m_store->Next(111);
	env.m_store->Decide(111, first.id, "like", 1);
	env.m_store->SetMode(111, first.id, "buy_side");
	env.m_store->SetOffset("bot", 13);
	env.m_store.reset();
	MatchStore reopened(env.m_path / "matches.sqlite3");
	reopened.Migrate();
	EXPECT_EQ(reopened.GetProgress(111).liked, 1);
	EXPECT_EQ(reopened.Next(111)->rank, 2);
	EXPECT_EQ(reopened.GetMode(111, first), "buy_side");
	EXPECT_EQ(reopened.GetOffset("bot"), 13);
}

TEST(MatchStoreTest, PaginatesFavorites)
{
	TestEnvironment env;
	for (int index = 1; index <= 7; ++index)
	{
		env.m_store->Decide(111, env.m_store->Next(111)->id, "like", index);
	}
	const auto page = env.m_store->Favorites(111, 5);
	ASSERT_EQ(page.size(), 2);
	EXPECT_EQ(page.front().rank, 6);
	EXPECT_EQ(page.back().rank, 7);
}

TEST(MatchStoreTest, FinishesFeed)
{
	TestEnvironment env;
	for (int index = 1; index <= 100; ++index)
	{
		env.m_store->Decide(111, env.m_store->Next(111)->id, "dislike", index);
	}
	EXPECT_FALSE(env.m_store->Next(111));
	EXPECT_EQ(env.m_store->GetProgress(111).skipped, 100);
	EXPECT_TRUE(env.m_store->Undo(111, env.m_store->LastDecision(111)->id, 101));
	EXPECT_EQ(env.m_store->Next(111)->rank, 100);
}

TEST(MatchStoreTest, RejectsBrokenCatalog)
{
	TestEnvironment env;
	Database database(env.m_path / "matches.sqlite3");
	database.Execute("DELETE FROM drafts WHERE id=(SELECT id FROM drafts LIMIT 1)");
	EXPECT_THROW(env.m_store->Validate(), std::runtime_error);
}

TEST(MatchStoreTest, RejectsWrongDatabase)
{
	TestEnvironment env;
	Database database(env.m_path / "matches.sqlite3");
	database.Execute("PRAGMA user_version=99");
	EXPECT_THROW(env.m_store->Validate(), std::runtime_error);
}

TEST(MatchStoreTest, KeepsOffsetMonotonic)
{
	TestEnvironment env;
	env.m_store->SetOffset("bot", 30);
	env.m_store->SetOffset("bot", 2);
	EXPECT_EQ(env.m_store->GetOffset("bot"), 30);
}
