#include <gtest/gtest.h>

#include "dispatcher/event_dispatcher.hpp"
#include "exchange/exchange_execution_listener.hpp"
#include "exchange/exchange_order_session_map.hpp"
#include "exchange/exchange_ouch_handler.hpp"
#include "execution/ouch/ouch_encoder.hpp"
#include "execution/ouch/ouch_messages.hpp"
#include "execution/ouch/ouch_response_dispatcher.hpp"
#include "orderbook/software/matching_engine.hpp"
#include "position/position_manager.hpp"

#include <variant>


TEST(
    ExchangeOuchHandlerTest,
    DecodedEnterOrdersReachMatchingEngineAndReturnAcceptedAndExecuted)
{
    PositionManager positionManager;

    ExchangeOrderSessionMap sessionMap;

    ExchangeExecutionListener executionListener(
        sessionMap);

    EventDispatcher dispatcher;

    dispatcher.addListener(
        &positionManager);

    dispatcher.addListener(
        &executionListener);

    MatchingEngine engine(
        dispatcher);

    ExchangeOuchHandler handler(
        engine,
        sessionMap);

    //
    // First order:
    // SELL 30 @ 100
    //
    ouch::EnterOrder sell{};

    sell.userRefNum = 101;
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

    const auto sellResponse =
        handler.handleEnterOrder(
            2001,
            sellBuffer.data(),
            sellBuffer.size());

    ASSERT_TRUE(
        sellResponse.has_value());

    const auto sellDispatchedResponse =
        ouch::OuchResponseDispatcher::dispatch(
            sellResponse->data(),
            sellResponse->size());

    ASSERT_TRUE(
        sellDispatchedResponse.has_value());

    ASSERT_TRUE(
        std::holds_alternative<ouch::Accepted>(
            *sellDispatchedResponse));

    const auto& sellAccepted =
        std::get<ouch::Accepted>(
            *sellDispatchedResponse);

    EXPECT_EQ(
        sellAccepted.userRefNum,
        sell.userRefNum);

    EXPECT_EQ(
        sellAccepted.side,
        Side::Sell);

    EXPECT_EQ(
        sellAccepted.quantity,
        30U);

    EXPECT_EQ(
        sellAccepted.price,
        100U);

    EXPECT_EQ(
        sellAccepted.symbol,
        sell.symbol);

    EXPECT_NE(
        sellAccepted.orderReferenceNumber,
        0U);

    const OrderId sellOrderId =
        sellAccepted.orderReferenceNumber;

    //
    // Second order:
    // BUY 30 @ 100
    //
    ouch::EnterOrder buy{};

    buy.userRefNum = 202;
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

    const auto buyResponse =
        handler.handleEnterOrder(
            1001,
            buyBuffer.data(),
            buyBuffer.size());

    ASSERT_TRUE(
        buyResponse.has_value());

    const auto buyDispatchedResponse =
        ouch::OuchResponseDispatcher::dispatch(
            buyResponse->data(),
            buyResponse->size());

    ASSERT_TRUE(
        buyDispatchedResponse.has_value());

    ASSERT_TRUE(
        std::holds_alternative<ouch::Accepted>(
            *buyDispatchedResponse));

    const auto& buyAccepted =
        std::get<ouch::Accepted>(
            *buyDispatchedResponse);

    EXPECT_EQ(
        buyAccepted.userRefNum,
        buy.userRefNum);

    EXPECT_EQ(
        buyAccepted.side,
        Side::Buy);

    EXPECT_EQ(
        buyAccepted.quantity,
        30U);

    EXPECT_EQ(
        buyAccepted.price,
        100U);

    EXPECT_EQ(
        buyAccepted.symbol,
        buy.symbol);

    EXPECT_NE(
        buyAccepted.orderReferenceNumber,
        0U);

    EXPECT_NE(
        buyAccepted.orderReferenceNumber,
        sellOrderId);

    //
    // The two orders should match.
    //
    EXPECT_EQ(
        positionManager.position(1001),
        30);

    EXPECT_EQ(
        positionManager.position(2001),
        -30);

    //
    // The trade listener should have produced
    // one Executed response for each side.
    //
    const auto& executionResponses =
        executionListener.lastResponses();

    ASSERT_TRUE(
        executionResponses.buyResponse.has_value());

    ASSERT_TRUE(
        executionResponses.sellResponse.has_value());

    //
    // Buyer execution response.
    //
    const auto buyExecutionResponse =
        ouch::OuchResponseDispatcher::dispatch(
            executionResponses.buyResponse->data(),
            executionResponses.buyResponse->size());

    ASSERT_TRUE(
        buyExecutionResponse.has_value());

    ASSERT_TRUE(
        std::holds_alternative<ouch::Executed>(
            *buyExecutionResponse));

    const auto& buyExecuted =
        std::get<ouch::Executed>(
            *buyExecutionResponse);

    EXPECT_EQ(
        buyExecuted.userRefNum,
        buy.userRefNum);

    EXPECT_EQ(
        buyExecuted.quantity,
        30U);

    EXPECT_EQ(
        buyExecuted.price,
        100U);

    EXPECT_NE(
        buyExecuted.matchNumber,
        0U);

    //
    // Seller execution response.
    //
    const auto sellExecutionResponse =
        ouch::OuchResponseDispatcher::dispatch(
            executionResponses.sellResponse->data(),
            executionResponses.sellResponse->size());

    ASSERT_TRUE(
        sellExecutionResponse.has_value());

    ASSERT_TRUE(
        std::holds_alternative<ouch::Executed>(
            *sellExecutionResponse));

    const auto& sellExecuted =
        std::get<ouch::Executed>(
            *sellExecutionResponse);

    EXPECT_EQ(
        sellExecuted.userRefNum,
        sell.userRefNum);

    EXPECT_EQ(
        sellExecuted.quantity,
        30U);

    EXPECT_EQ(
        sellExecuted.price,
        100U);

    EXPECT_NE(
        sellExecuted.matchNumber,
        0U);

    //
    // Both executions represent the same trade.
    //
    EXPECT_EQ(
        buyExecuted.matchNumber,
        sellExecuted.matchNumber);
}
