#include <gtest/gtest.h>

#include "dispatcher/event_dispatcher.hpp"
#include "exchange/exchange_ouch_handler.hpp"
#include "execution/ouch/ouch_encoder.hpp"
#include "execution/ouch/ouch_messages.hpp"
#include "orderbook/software/matching_engine.hpp"
#include "position/position_manager.hpp"

TEST(
    ExchangeOuchHandlerTest,
    DecodedEnterOrdersReachMatchingEngineAndTrade)
{
    PositionManager positionManager;

    EventDispatcher dispatcher;

    dispatcher.addListener(
        &positionManager);

    MatchingEngine engine(
        dispatcher);

    ExchangeOuchHandler handler(
        engine);

    //
    // First order:
    // SELL 30 @ 100
    //
    ouch::EnterOrder sell{};

    sell.userRefNum = 1;
    sell.side = Side::Sell;
    sell.quantity = 30;
    sell.price = 100;

    sell.symbol = {
        'A', 'A', 'P', 'L',
        ' ', ' ', ' ', ' '
    };

    const auto sellBuffer =
        ouch::OuchEncoder::encode(
            sell);

    ASSERT_TRUE(
        handler.handleEnterOrder(
            2001,
            sellBuffer.data(),
            sellBuffer.size()));

    //
    // Second order:
    // BUY 30 @ 100
    //
    ouch::EnterOrder buy{};

    buy.userRefNum = 2;
    buy.side = Side::Buy;
    buy.quantity = 30;
    buy.price = 100;

    buy.symbol = {
        'A', 'A', 'P', 'L',
        ' ', ' ', ' ', ' '
    };

    const auto buyBuffer =
        ouch::OuchEncoder::encode(
            buy);

    ASSERT_TRUE(
        handler.handleEnterOrder(
            1001,
            buyBuffer.data(),
            buyBuffer.size()));

    //
    // If the OUCH bytes were decoded correctly
    // and submitted into MatchingEngine,
    // BUY and SELL should match for quantity 30.
    //
    EXPECT_EQ(
        positionManager.position(1001),
        30);

    EXPECT_EQ(
        positionManager.position(2001),
        -30);
}
