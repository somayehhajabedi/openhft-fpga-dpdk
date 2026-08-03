#include <gtest/gtest.h>

#include "common/numa_utils.hpp"

TEST(NumaUtilsTest, NumaIsAvailable)
{
    EXPECT_TRUE(
        numa_utils::isAvailable());
}

TEST(NumaUtilsTest, NodeCountIsPositive)
{
    EXPECT_GE(
        numa_utils::nodeCount(),
        1);
}

TEST(NumaUtilsTest, CurrentCpuNodeIsValid)
{
    EXPECT_GE(
        numa_utils::currentCpuNode(),
        0);
}

TEST(NumaUtilsTest, ValidNode)
{
    EXPECT_TRUE(
        numa_utils::isValidNode(0));
}

TEST(NumaUtilsTest, InvalidNode)
{
    EXPECT_FALSE(
        numa_utils::isValidNode(999));
}

TEST(NumaUtilsTest, AllocateAndFreeMemory)
{
    constexpr std::size_t Size = 4096;

    void* memory =
        numa_utils::allocateOnNode(
            Size,
            0);

    ASSERT_NE(
        memory,
        nullptr);

    numa_utils::freeMemory(
        memory,
        Size);
}

TEST(NumaUtilsTest, InvalidNodeAllocationFails)
{
    void* memory =
        numa_utils::allocateOnNode(
            4096,
            999);

    EXPECT_EQ(
        memory,
        nullptr);
}

