#include <gtest/gtest.h>

#include "dispatcher/event_dispatcher.hpp"
#include "gateway/gateway.hpp"
#include "orderbook/software/matching_engine.hpp"
#include "orderbook/software/order.hpp"
#include "risk/risk_manager.hpp"
#include "risk/risk_result.hpp"

namespace
{

class GatewayTest : public ::testing::Test
{
protected:
    EventDispatcher dispatcher;
    MatchingEngine engine{dispatcher};
    RiskManager riskManager;
    Gateway gateway{engine, riskManager};
};

Order makeOrder(OrderId id,
                AccountId accountId,
                Side side,
                Price price,
                Quantity quantity)
{
    return Order{
        .id = id,
        .account_id = accountId,
        .side = side,
        .price = price,
        .quantity = quantity
    };
}

} // namespace

TEST_F(GatewayTest, AcceptsValidOrder)
{
    Order order = makeOrder(
        1,
        1001,
        Side::Buy,
        100,
        100);

    EXPECT_EQ(
        gateway.submit(&order),
        RiskResult::Accepted);
}

TEST_F(GatewayTest, RejectsNullOrder)
{
    EXPECT_NE(
        gateway.submit(nullptr),
        RiskResult::Accepted);
}

TEST_F(GatewayTest, RejectsZeroPrice)
{
    Order order = makeOrder(
        1,
        1001,
        Side::Buy,
        0,
        100);

    EXPECT_EQ(
        gateway.submit(&order),
        RiskResult::InvalidPrice);
}

TEST_F(GatewayTest, RejectsZeroQuantity)
{
    Order order = makeOrder(
        1,
        1001,
        Side::Buy,
        100,
        0);

    EXPECT_EQ(
        gateway.submit(&order),
        RiskResult::InvalidQuantity);
}

TEST_F(GatewayTest, RejectsOrderAboveQuantityLimit)
{
    Order order = makeOrder(
        1,
        1001,
        Side::Buy,
        1,
        100001);

    EXPECT_EQ(
        gateway.submit(&order),
        RiskResult::MaxOrderQuantityExceeded);
}

TEST_F(GatewayTest, RejectsOrderWhenCumulativePositionExceedsLimit)
{
    Order firstOrder = makeOrder(
        1,
        1001,
        Side::Buy,
        100,
        400);

    Order secondOrder = makeOrder(
        2,
        1001,
        Side::Buy,
        100,
        200);

    EXPECT_EQ(
        gateway.submit(&firstOrder),
        RiskResult::Accepted);

    EXPECT_EQ(
        gateway.submit(&secondOrder),
        RiskResult::MaxPositionExceeded);
}