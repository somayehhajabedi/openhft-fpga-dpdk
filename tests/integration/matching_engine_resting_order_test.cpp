#include <gtest/gtest.h>

#include "dispatcher/event_dispatcher.hpp"
#include "orderbook/software/matching_engine.hpp"
#include "position/position_manager.hpp"

// Verifies that non-crossing orders remain resting in the order book.
TEST(MatchingEngineRestingOrderTest, AddsNonCrossingOrderToBook)
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
        .quantity = 50
    };

    Order buy{
        .id = 2,
        .account_id = 1001,
        .side = Side::Buy,
        .price = 99,
        .quantity = 20
    };

    engine.process(&sell);
    engine.process(&buy);

    EXPECT_EQ(sell.quantity, 50);
    EXPECT_EQ(buy.quantity, 20);

    EXPECT_EQ(position_manager.position(1001), 0);
    EXPECT_EQ(position_manager.position(2001), 0);
}