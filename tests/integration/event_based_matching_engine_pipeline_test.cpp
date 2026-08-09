/*
 * Event-Based Matching Engine Pipeline Integration Tests
 *
 * Purpose
 * -------
 * Verifies the complete event-driven processing path from the
 * MarketDataPipeline to the Matching Engine.
 *
 * The test exercises the following flow:
 *
 *     MarketDataEvent
 *             │
 *             ▼
 *     MarketDataPipeline
 *             │
 *             ▼
 *        Dispatcher
 *             │
 *             ▼
 *  MatchingEngineConsumer
 *             │
 *             ▼
 *      MatchingEngine
 *             │
 *             ▼
 *       ArrayOrderBook
 *             │
 *             ▼
 *      EventDispatcher
 *             │
 *             ▼
 *     PositionManager
 *
 * Scenario
 * --------
 * 1. Add a sell order for 50 units at price 100.
 * 2. Cancel 20 units, leaving 30 units resting.
 * 3. Add a crossing buy order for 40 units at price 100.
 * 4. Verify that 30 units are traded.
 *
 * Expected positions:
 *
 *     Buyer  = +30
 *     Seller = -30
 */

#include <gtest/gtest.h>

#include "dispatcher/event_dispatcher.hpp"
#include "orderbook/software/matching_engine.hpp"
#include "pipeline/market_data_event.hpp"
#include "pipeline/market_data_pipeline.hpp"
#include "pipeline/matching_engine_consumer.hpp"
#include "position/position_manager.hpp"

#include <chrono>
#include <thread>

TEST(
    EventBasedMatchingEnginePipelineTest,
    ProcessesAddCancelAndMatchEndToEnd)
{
    using namespace std::chrono_literals;

    constexpr AccountId BuyerAccountId = 1001;
    constexpr AccountId SellerAccountId = 2001;

    PositionManager positionManager;

    EventDispatcher eventDispatcher;
    eventDispatcher.addListener(&positionManager);

    MatchingEngine matchingEngine(eventDispatcher);

    MatchingEngineConsumer consumer(matchingEngine);

    MarketDataPipeline pipeline(consumer);

    pipeline.start();

    const MarketDataEvent addSell{
        .type = MarketDataEventType::AddOrder,
        .orderId = 1,
        .newOrderId = 0,
        .accountId = SellerAccountId,
        .side = Side::Sell,
        .price = 100,
        .quantity = 50
    };

    ASSERT_TRUE(
        pipeline.submit(addSell));

    const MarketDataEvent cancelSell{
        .type = MarketDataEventType::CancelOrder,
        .orderId = 1,
        .newOrderId = 0,
        .accountId = 0,
        .side = Side::Sell,
        .price = 0,
        .quantity = 20
    };

    ASSERT_TRUE(
        pipeline.submit(cancelSell));

    const MarketDataEvent addBuy{
        .type = MarketDataEventType::AddOrder,
        .orderId = 2,
        .newOrderId = 0,
        .accountId = BuyerAccountId,
        .side = Side::Buy,
        .price = 100,
        .quantity = 40
    };

    ASSERT_TRUE(
        pipeline.submit(addBuy));

    const auto deadline =
        std::chrono::steady_clock::now() + 100ms;

    while (
        positionManager.position(BuyerAccountId) != 30 &&
        std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }

    pipeline.stop();

    EXPECT_EQ(
        positionManager.position(BuyerAccountId),
        30);

    EXPECT_EQ(
        positionManager.position(SellerAccountId),
        -30);
}