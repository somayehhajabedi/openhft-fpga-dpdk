#include <iostream>

#include "dispatcher/event_dispatcher.hpp"
#include "gateway/gateway.hpp"
#include "journal/journal.hpp"
#include "market_data/trade_publisher.hpp"
#include "orderbook/software/matching_engine.hpp"
#include "orderbook/software/order.hpp"
#include "position/position_manager.hpp"
#include "risk/risk_manager.hpp"
#include "risk/risk_result.hpp"

int main()
{
    Journal journal;
    TradePublisher trade_publisher;
    PositionManager position_manager;

    EventDispatcher dispatcher;
    dispatcher.addListener(&journal);
    dispatcher.addListener(&trade_publisher);
    dispatcher.addListener(&position_manager);

    MatchingEngine engine(dispatcher);

    RiskManager risk_manager;
    Gateway gateway(engine, risk_manager);

    Order sell1{
        .id = 1,
        .account_id = 2001,
        .side = Side::Sell,
        .price = 100,
        .quantity = 30
    };

    Order sell2{
        .id = 2,
        .account_id = 2001,
        .side = Side::Sell,
        .price = 101,
        .quantity = 50
    };

    Order buy{
        .id = 3,
        .account_id = 1001,
        .side = Side::Buy,
        .price = 101,
        .quantity = 30
    };

    [[maybe_unused]] const RiskResult sell1_result =
        gateway.submit(&sell1);

    [[maybe_unused]] const RiskResult sell2_result =
        gateway.submit(&sell2);

    [[maybe_unused]] const RiskResult buy_result =
        gateway.submit(&buy);

    std::cout << "sell1 remaining: " << sell1.quantity << '\n';
    std::cout << "sell2 remaining: " << sell2.quantity << '\n';
    std::cout << "buy remaining: " << buy.quantity << '\n';

    std::cout << "Buyer Position: "
              << position_manager.position(1001)
              << '\n';

    std::cout << "Seller Position: "
              << position_manager.position(2001)
              << '\n';

    return 0;
}