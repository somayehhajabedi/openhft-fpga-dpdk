#include <gtest/gtest.h>

#include "dispatcher/event_dispatcher.hpp"
#include "orderbook/software/matching_engine.hpp"
#include "position/position_manager.hpp"

// Verifies that unmatched incoming quantity rests in the order book after a partial fill.
TEST(MatchingEnginePartialIncomingTest, RestsRemainingIncomingQuantityAfterPartialFill)
{
    PositionManager position_manager;

    EventDispatcher dispatcher;
    dispatcher.addListener(&position_manager);

    MatchingEngine engine(dispatcher);

    Order sell{
        .id = 1,
        .account_id = 2001,
        .side = Side::Sell,
        .price = 100,
        .quantity = 30
    };

    Order buy{
        .id = 2,
        .account_id = 1001,
        .side = Side::Buy,
        .price = 100,
        .quantity = 50
    };

    engine.process(&sell);
    engine.process(&buy);

    EXPECT_EQ(sell.quantity, 0);
    EXPECT_EQ(buy.quantity, 20);

    EXPECT_EQ(position_manager.position(1001), 30);
    EXPECT_EQ(position_manager.position(2001), -30);
}