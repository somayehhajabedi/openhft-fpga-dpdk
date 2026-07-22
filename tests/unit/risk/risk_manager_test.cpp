#include <gtest/gtest.h>

#include "risk/risk_manager.hpp"

TEST(RiskManagerTest, AcceptValidOrder)
{
    RiskManager risk;

    Order order{
        .id = 1,
        .account_id = 1,
        .side = Side::Buy,
        .price = 100,
        .quantity = 10
    };

    EXPECT_EQ(
        risk.check(&order),
        RiskResult::Accepted);
}
TEST(RiskManagerTest, RejectZeroPrice)
{
    RiskManager risk;

    Order order{
        .id = 1,
        .account_id = 1,
        .side = Side::Buy,
        .price = 0,
        .quantity = 10
    };

    EXPECT_EQ(risk.check(&order), RiskResult::InvalidPrice);
}

TEST(RiskManagerTest, RejectZeroQuantity)
{
    RiskManager risk;

    Order order{
        .id = 1,
        .account_id = 1,
        .side = Side::Buy,
        .price = 100,
        .quantity = 0
    };

    EXPECT_EQ(risk.check(&order), RiskResult::InvalidQuantity);
}

TEST(RiskManagerTest, RejectTooLargeQuantity)
{
    RiskManager risk;

    Order order{
        .id = 1,
        .account_id = 1,
        .side = Side::Buy,
        .price = 100,
        .quantity = 100001
    };

    EXPECT_EQ(
        risk.check(&order),
        RiskResult::MaxOrderQuantityExceeded);
}

TEST(RiskManagerTest, RejectTooLargeOrderValue)
{
    RiskManager risk;

    Order order{
        .id = 1,
        .account_id = 1,
        .side = Side::Buy,
        .price = 100000,
        .quantity = 101
    };

    EXPECT_EQ(
        risk.check(&order),
        RiskResult::MaxOrderValueExceeded);
}

TEST(RiskManagerTest, RejectPositionLimit)
{
    RiskManager risk;

    Order acceptedOrder{
        .id = 1,
        .account_id = 1,
        .side = Side::Buy,
        .price = 100,
        .quantity = 450
    };

    EXPECT_EQ(
        risk.check(&acceptedOrder),
        RiskResult::Accepted);

    risk.onAccepted(&acceptedOrder);

    Order nextOrder{
        .id = 2,
        .account_id = 1,
        .side = Side::Buy,
        .price = 100,
        .quantity = 100
    };

    EXPECT_EQ(
        risk.check(&nextOrder),
        RiskResult::MaxPositionExceeded);
}
TEST(RiskManagerTest, RejectNullOrder)
{
    RiskManager risk;

    EXPECT_EQ(
        risk.check(nullptr),
        RiskResult::InvalidQuantity);
}