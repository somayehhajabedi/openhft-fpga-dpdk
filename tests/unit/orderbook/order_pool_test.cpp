#include <gtest/gtest.h>

#include "order_pool.hpp"

TEST(OrderPoolTest, InitiallyAllOrdersAreAvailable)
{
    OrderPool pool(4);

    EXPECT_EQ(pool.capacity(), 4);
    EXPECT_EQ(pool.available(), 4);
}

TEST(OrderPoolTest, AcquireDecreasesAvailable)
{
    OrderPool pool(4);

    Order* order = pool.acquire();

    ASSERT_NE(order, nullptr);
    EXPECT_EQ(pool.available(), 3);
}

TEST(OrderPoolTest, AcquireReturnsNullptrWhenPoolIsEmpty)
{
    OrderPool pool(2);

    EXPECT_NE(pool.acquire(), nullptr);
    EXPECT_NE(pool.acquire(), nullptr);
    EXPECT_EQ(pool.acquire(), nullptr);
}

TEST(OrderPoolTest, ReleaseMakesOrderAvailableAgain)
{
    OrderPool pool(1);

    Order* order = pool.acquire();

    ASSERT_NE(order, nullptr);

    pool.release(order);

    EXPECT_EQ(pool.available(), 1);
}

TEST(OrderPoolTest, ReleaseUsesLifoOrder)
{
    OrderPool pool(2);

    Order* first = pool.acquire();
    Order* second = pool.acquire();

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    pool.release(first);
    pool.release(second);

    EXPECT_EQ(pool.acquire(), second);
    EXPECT_EQ(pool.acquire(), first);
}

TEST(OrderPoolTest, OwnsAcquiredOrder)
{
    OrderPool pool(2);

    Order* order = pool.acquire();

    EXPECT_TRUE(pool.owns(order));
}

TEST(OrderPoolTest, DoesNotOwnExternalOrder)
{
    OrderPool pool(2);

    Order externalOrder{};

    EXPECT_FALSE(pool.owns(&externalOrder));
}

TEST(OrderPoolTest, DoesNotOwnNullptr)
{
    OrderPool pool(2);

    EXPECT_FALSE(pool.owns(nullptr));
}
