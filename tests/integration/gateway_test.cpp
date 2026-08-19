#include <gtest/gtest.h>

#include "gateway/gateway.hpp"
#include "gateway/order_execution_sink.hpp"
#include "models/order_intent.hpp"
#include "risk/risk_manager.hpp"
#include "risk/risk_result.hpp"

namespace
{

class RecordingExecutionSink final : public OrderExecutionSink
{
public:
    bool submit(
        const OrderIntent& intent) override
    {
        lastIntent = intent;
        submitted = true;

        return acceptSubmission;
    }

    OrderIntent lastIntent{};
    bool submitted{false};
    bool acceptSubmission{true};
};

class GatewayTest : public ::testing::Test
{
protected:
    RiskManager riskManager;

    RecordingExecutionSink executionSink;

    Gateway gateway{
        riskManager,
        executionSink};
};

OrderIntent makeIntent(
    AccountId accountId,
    Side side,
    Price price,
    Quantity quantity)
{
    return OrderIntent{
        .accountId = accountId,
        .side = side,
        .price = price,
        .quantity = quantity
    };
}

} // namespace

TEST_F(GatewayTest, AcceptsValidOrder)
{
    const OrderIntent intent =
        makeIntent(
            1001,
            Side::Buy,
            100,
            100);

    const GatewayResult result =
        gateway.submit(intent);

    EXPECT_TRUE(
        result.accepted());

    EXPECT_EQ(
        result.riskResult,
        RiskResult::Accepted);

    EXPECT_TRUE(
        result.executionSucceeded);

    ASSERT_TRUE(
        executionSink.submitted);

    EXPECT_EQ(
        executionSink.lastIntent.accountId,
        1001U);

    EXPECT_EQ(
        executionSink.lastIntent.side,
        Side::Buy);

    EXPECT_EQ(
        executionSink.lastIntent.price,
        100);

    EXPECT_EQ(
        executionSink.lastIntent.quantity,
        100U);
}

TEST_F(GatewayTest, RejectsZeroPrice)
{
    const OrderIntent intent =
        makeIntent(
            1001,
            Side::Buy,
            0,
            100);

    const GatewayResult result =
        gateway.submit(intent);

    EXPECT_FALSE(
        result.accepted());

    EXPECT_EQ(
        result.riskResult,
        RiskResult::InvalidPrice);

    EXPECT_FALSE(
        result.executionSucceeded);

    EXPECT_FALSE(
        executionSink.submitted);
}

TEST_F(GatewayTest, RejectsZeroQuantity)
{
    const OrderIntent intent =
        makeIntent(
            1001,
            Side::Buy,
            100,
            0);

    const GatewayResult result =
        gateway.submit(intent);

    EXPECT_FALSE(
        result.accepted());

    EXPECT_EQ(
        result.riskResult,
        RiskResult::InvalidQuantity);

    EXPECT_FALSE(
        result.executionSucceeded);

    EXPECT_FALSE(
        executionSink.submitted);
}

TEST_F(GatewayTest, RejectsOrderAboveQuantityLimit)
{
    const OrderIntent intent =
        makeIntent(
            1001,
            Side::Buy,
            1,
            100001);

    const GatewayResult result =
        gateway.submit(intent);

    EXPECT_FALSE(
        result.accepted());

    EXPECT_EQ(
        result.riskResult,
        RiskResult::MaxOrderQuantityExceeded);

    EXPECT_FALSE(
        result.executionSucceeded);

    EXPECT_FALSE(
        executionSink.submitted);
}

TEST_F(
    GatewayTest,
    RejectsOrderWhenCumulativePositionExceedsLimit)
{
    const OrderIntent firstIntent =
        makeIntent(
            1001,
            Side::Buy,
            100,
            400);

    const OrderIntent secondIntent =
        makeIntent(
            1001,
            Side::Buy,
            100,
            200);

    const GatewayResult firstResult =
        gateway.submit(firstIntent);

    EXPECT_TRUE(
        firstResult.accepted());

    executionSink.submitted = false;

    const GatewayResult secondResult =
        gateway.submit(secondIntent);

    EXPECT_FALSE(
        secondResult.accepted());

    EXPECT_EQ(
        secondResult.riskResult,
        RiskResult::MaxPositionExceeded);

    EXPECT_FALSE(
        secondResult.executionSucceeded);

    EXPECT_FALSE(
        executionSink.submitted);
}

TEST_F(
    GatewayTest,
    DoesNotUpdateRiskWhenExecutionRejectsOrder)
{
    executionSink.acceptSubmission = false;

    const OrderIntent firstIntent =
        makeIntent(
            1001,
            Side::Buy,
            100,
            400);

    const GatewayResult firstResult =
        gateway.submit(firstIntent);

    EXPECT_FALSE(
        firstResult.accepted());

    EXPECT_EQ(
        firstResult.riskResult,
        RiskResult::Accepted);

    EXPECT_FALSE(
        firstResult.executionSucceeded);

    executionSink.acceptSubmission = true;
    executionSink.submitted = false;

    const OrderIntent secondIntent =
        makeIntent(
            1001,
            Side::Buy,
            100,
            200);

    const GatewayResult secondResult =
        gateway.submit(secondIntent);

    EXPECT_TRUE(
        secondResult.accepted());

    EXPECT_EQ(
        secondResult.riskResult,
        RiskResult::Accepted);

    EXPECT_TRUE(
        secondResult.executionSucceeded);
}