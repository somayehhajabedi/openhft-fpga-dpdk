#include <gtest/gtest.h>

#include "dispatcher/event_dispatcher.hpp"
#include "orderbook/software/matching_engine.hpp"
#include "position/position_manager.hpp"

TEST(MatchingEngineFifoTest, MatchesOrdersAtSamePriceInArrivalOrder)
{
    PositionManager position_manager;

    EventDispatcher dispatcher;
    dispatcher.addListener(&position_manager);

    MatchingEngine engine(dispatcher);

    Order sell1{
        .id = 1,
        .account_id = 2001,
        .side = Side::Sell,
        .price = 100,
        .quantity = 30
    };

    Order sell2{
        .id = 2,
        .account_id = 2002,
        .side = Side::Sell,
        .price = 100,
        .quantity = 50
    };

    Order buy{
        .id = 3,
        .account_id = 1001,
        .side = Side::Buy,
        .price = 100,
        .quantity = 40
    };

    engine.process(&sell1);
    engine.process(&sell2);
    engine.process(&buy);

    EXPECT_EQ(sell1.quantity, 0);
    EXPECT_EQ(sell2.quantity, 40);
    EXPECT_EQ(buy.quantity, 0);

    EXPECT_EQ(position_manager.position(1001), 40);
    EXPECT_EQ(position_manager.position(2001), -30);
    EXPECT_EQ(position_manager.position(2002), -10);
}