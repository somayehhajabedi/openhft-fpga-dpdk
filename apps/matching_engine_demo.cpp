#include <iostream>

#include "dispatcher/event_dispatcher.hpp"
#include "gateway/gateway.hpp"
#include "gateway/matching_engine_execution_sink.hpp"
#include "journal/journal.hpp"
#include "market_data/trade_publisher.hpp"
#include "models/order_intent.hpp"
#include "orderbook/software/matching_engine.hpp"
#include "position/position_manager.hpp"
#include "risk/risk_manager.hpp"

int main()
{
    Journal journal;
    TradePublisher tradePublisher;
    PositionManager positionManager;

    EventDispatcher dispatcher;

    dispatcher.addListener(&journal);
    dispatcher.addListener(&tradePublisher);
    dispatcher.addListener(&positionManager);

    MatchingEngine engine(
        dispatcher);

    MatchingEngineExecutionSink executionSink(
        engine);

    RiskManager riskManager;

    Gateway gateway(
        riskManager,
        executionSink);

    const OrderIntent sell1{
        .accountId = 2001,
        .side = Side::Sell,
        .price = 100,
        .quantity = 30
    };

    const OrderIntent sell2{
        .accountId = 2001,
        .side = Side::Sell,
        .price = 101,
        .quantity = 50
    };

    const OrderIntent buy{
        .accountId = 1001,
        .side = Side::Buy,
        .price = 101,
        .quantity = 30
    };

    [[maybe_unused]]
    const GatewayResult sell1Result =
        gateway.submit(
            sell1);

    [[maybe_unused]]
    const GatewayResult sell2Result =
        gateway.submit(
            sell2);

    [[maybe_unused]]
    const GatewayResult buyResult =
        gateway.submit(
            buy);

    std::cout
        << "Buyer Position: "
        << positionManager.position(
               1001)
        << '\n';

    std::cout
        << "Seller Position: "
        << positionManager.position(
               2001)
        << '\n';

    return 0;
}