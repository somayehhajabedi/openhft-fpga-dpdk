
#include <gtest/gtest.h>

#include "models/order_intent.hpp"
#include "risk/risk_manager.hpp"

TEST(RiskManagerTest, AcceptValidOrder)
{
    RiskManager risk;

    const OrderIntent intent{
        .accountId = 1001,
        .side = Side::Buy,
        .price = 100,
        .quantity = 100
    };

    EXPECT_EQ(
        risk.check(intent),
        RiskResult::Accepted);
}

TEST(RiskManagerTest, RejectZeroPrice)
{
    RiskManager risk;

    const OrderIntent intent{
        .accountId = 1001,
        .side = Side::Buy,
        .price = 0,
        .quantity = 100
    };

    EXPECT_EQ(
        risk.check(intent),
        RiskResult::InvalidPrice);
}

TEST(RiskManagerTest, RejectZeroQuantity)
{
    RiskManager risk;

    const OrderIntent intent{
        .accountId = 1001,
        .side = Side::Buy,
        .price = 100,
        .quantity = 0
    };

    EXPECT_EQ(
        risk.check(intent),
        RiskResult::InvalidQuantity);
}

TEST(RiskManagerTest, RejectTooLargeQuantity)
{
    RiskManager risk;

    const OrderIntent intent{
        .accountId = 1001,
        .side = Side::Buy,
        .price = 100,
        .quantity = 100001
    };

    EXPECT_EQ(
        risk.check(intent),
        RiskResult::MaxOrderQuantityExceeded);
}

TEST(RiskManagerTest, RejectTooLargeOrderValue)
{
    RiskManager risk;

    const OrderIntent intent{
        .accountId = 1001,
        .side = Side::Buy,
        .price = 100001,
        .quantity = 101
    };

    EXPECT_EQ(
        risk.check(intent),
        RiskResult::MaxOrderValueExceeded);
}

TEST(RiskManagerTest, RejectPositionLimit)
{
    RiskManager risk;

    const OrderIntent acceptedIntent{
        .accountId = 1001,
        .side = Side::Buy,
        .price = 100,
        .quantity = 400
    };

    ASSERT_EQ(
        risk.check(acceptedIntent),
        RiskResult::Accepted);

    risk.onAccepted(acceptedIntent);

    const OrderIntent nextIntent{
        .accountId = 1001,
        .side = Side::Buy,
        .price = 100,
        .quantity = 101
    };

    EXPECT_EQ(
        risk.check(nextIntent),
        RiskResult::MaxPositionExceeded);
}