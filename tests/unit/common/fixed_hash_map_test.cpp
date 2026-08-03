#include <gtest/gtest.h>

#include "../common/fixed_hash_map.hpp"

TEST(FixedHashMapTest, InsertAndFind)
{
    FixedHashMap<int, int, 16> map;

    EXPECT_TRUE(map.insert(1, 100));

    auto* value = map.find(1);

    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 100);
}

TEST(FixedHashMapTest, Contains)
{
    FixedHashMap<int, int, 16> map;

    map.insert(5, 42);

    EXPECT_TRUE(map.contains(5));
    EXPECT_FALSE(map.contains(99));
}

TEST(FixedHashMapTest, UpdateExistingKey)
{
    FixedHashMap<int, int, 16> map;

    map.insert(7, 10);
    map.insert(7, 20);

    auto* value = map.find(7);

    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 20);

    EXPECT_EQ(map.size(), 1u);
}

TEST(FixedHashMapTest, Erase)
{
    FixedHashMap<int, int, 16> map;

    map.insert(3, 33);

    EXPECT_TRUE(map.erase(3));

    EXPECT_FALSE(map.contains(3));
    EXPECT_EQ(map.size(), 0u);
}

TEST(FixedHashMapTest, Collision)
{
    FixedHashMap<int, int, 4> map;

    EXPECT_TRUE(map.insert(1, 10));
    EXPECT_TRUE(map.insert(5, 20));
    EXPECT_TRUE(map.insert(9, 30));

    EXPECT_EQ(*map.find(1), 10);
    EXPECT_EQ(*map.find(5), 20);
    EXPECT_EQ(*map.find(9), 30);
}

TEST(FixedHashMapTest, ReuseDeletedSlot)
{
    FixedHashMap<int, int, 4> map;

    map.insert(1, 10);
    map.insert(5, 20);

    EXPECT_TRUE(map.erase(1));

    EXPECT_TRUE(map.insert(9, 30));

    EXPECT_EQ(*map.find(5), 20);
    EXPECT_EQ(*map.find(9), 30);
}

TEST(FixedHashMapTest, FullTable)
{
    FixedHashMap<int, int, 2> map;

    EXPECT_TRUE(map.insert(1, 1));
    EXPECT_TRUE(map.insert(2, 2));

    EXPECT_FALSE(map.insert(3, 3));
}
TEST(FixedHashMapTest, UpdatesExistingKeyBeyondDeletedSlot)
{
    FixedHashMap<int, int, 4> map;

    EXPECT_TRUE(map.insert(1, 10));
    EXPECT_TRUE(map.insert(5, 20));

    EXPECT_TRUE(map.erase(1));

    EXPECT_TRUE(map.insert(5, 30));

    const auto* value = map.find(5);

    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 30);
    EXPECT_EQ(map.size(), 1u);
}
TEST(FixedHashMapTest, EraseMissingKeyReturnsFalse)
{
    FixedHashMap<int, int, 4> map;

    EXPECT_FALSE(map.erase(99));
    EXPECT_EQ(map.size(), 0u);
}
TEST(FixedHashMapTest, ReusesDeletedEntriesAfterBecomingEmpty)
{
    FixedHashMap<int, int, 4> map;

    EXPECT_TRUE(map.insert(0, 10));
    EXPECT_TRUE(map.insert(4, 20));
    EXPECT_TRUE(map.insert(8, 30));
    EXPECT_TRUE(map.insert(12, 40));

    EXPECT_TRUE(map.erase(0));
    EXPECT_TRUE(map.erase(4));
    EXPECT_TRUE(map.erase(8));
    EXPECT_TRUE(map.erase(12));

    EXPECT_TRUE(map.empty());

    EXPECT_TRUE(map.insert(16, 50));

    const auto* value = map.find(16);

    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 50);
    EXPECT_EQ(map.size(), 1u);
}
