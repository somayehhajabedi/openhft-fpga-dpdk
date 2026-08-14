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

    EXPECT_EQ(
        gateway.submit(intent),
        RiskResult::Accepted);

    ASSERT_TRUE(executionSink.submitted);

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

    EXPECT_EQ(
        gateway.submit(intent),
        RiskResult::InvalidPrice);

    EXPECT_FALSE(executionSink.submitted);
}

TEST_F(GatewayTest, RejectsZeroQuantity)
{
    const OrderIntent intent =
        makeIntent(
            1001,
            Side::Buy,
            100,
            0);

    EXPECT_EQ(
        gateway.submit(intent),
        RiskResult::InvalidQuantity);

    EXPECT_FALSE(executionSink.submitted);
}

TEST_F(GatewayTest, RejectsOrderAboveQuantityLimit)
{
    const OrderIntent intent =
        makeIntent(
            1001,
            Side::Buy,
            1,
            100001);

    EXPECT_EQ(
        gateway.submit(intent),
        RiskResult::MaxOrderQuantityExceeded);

    EXPECT_FALSE(executionSink.submitted);
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

    EXPECT_EQ(
        gateway.submit(firstIntent),
        RiskResult::Accepted);

    executionSink.submitted = false;

    EXPECT_EQ(
        gateway.submit(secondIntent),
        RiskResult::MaxPositionExceeded);

    EXPECT_FALSE(executionSink.submitted);
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

    EXPECT_EQ(
        gateway.submit(firstIntent),
        RiskResult::Accepted);

    executionSink.acceptSubmission = true;
    executionSink.submitted = false;

    const OrderIntent secondIntent =
        makeIntent(
            1001,
            Side::Buy,
            100,
            200);

    EXPECT_EQ(
        gateway.submit(secondIntent),
        RiskResult::Accepted);
}
