#include <iostream>
#include "matching_engine.hpp"
#include "../../gateway/gateway.hpp"
#include "../../market_data/trade_publisher.hpp"
#include "../../dispatcher/event_dispatcher.hpp"
#include "../../journal/journal.hpp"
#include "../../position/position_manager.hpp"



MatchingEngine::MatchingEngine(EventDispatcher& dispatcher)
    : dispatcher_(dispatcher)
{
}

void MatchingEngine::process(Order* order)
{
    if (canCross(order))
    {
        executeTrade(order);
    }
    else
    {
        book_.addOrder(order);
    }
}

bool MatchingEngine::canCross(const Order* order) const
{
    if (order->side == Side::Buy)
    {
        const PriceLevel* best_ask = book_.bestAsk();

        return best_ask && order->price >= best_ask->price;
    }

    const PriceLevel* best_bid = book_.bestBid();

    return best_bid && order->price <= best_bid->price;
}

void MatchingEngine::executeTrade(Order* incoming)
{
    while (incoming->quantity > 0 && canCross(incoming))
    {
        if (!matchOne(incoming))
            break;
    }

    if (incoming->quantity > 0)
        book_.addOrder(incoming);
}

bool MatchingEngine::matchOne(Order* incoming)
{
    const PriceLevel* opposite_level =
        incoming->side == Side::Buy ? book_.bestAsk() : book_.bestBid();

    if (!opposite_level || !opposite_level->front())
        return false;

    Order* resting = opposite_level->front();

    Quantity traded_quantity =
        incoming->quantity < resting->quantity
            ? incoming->quantity
            : resting->quantity;

    [[maybe_unused]] Trade trade = createTrade(incoming, resting, traded_quantity);

    dispatcher_.publish(trade);

    /*std::cout << "TRADE: "
          << "BUY=" << trade.buy_order_id
          << " SELL=" << trade.sell_order_id
          << " PRICE=" << trade.price
          << " QTY=" << trade.quantity
          << std::endl;*/

    incoming->quantity -= traded_quantity;
    resting->quantity -= traded_quantity;

    if (resting->level)
        resting->level->total_quantity -= traded_quantity;

    if (resting->quantity == 0)
        book_.cancelOrder(resting->id);

    return traded_quantity > 0;
}


Trade MatchingEngine::createTrade(const Order* incoming,
                                  const Order* resting,
                                  Quantity traded_quantity)
{
    Trade trade{};

    if (incoming->side == Side::Buy)       
    {
       trade.buy_order_id = incoming->id;
       trade.sell_order_id = resting->id;

       trade.buy_account_id = incoming->account_id;
       trade.sell_account_id = resting->account_id;
    }   
    else
    {
       trade.buy_order_id = resting->id;
       trade.sell_order_id = incoming->id;

       trade.buy_account_id = resting->account_id;
       trade.sell_account_id = incoming->account_id;
    }

    trade.price = resting->price;
    trade.quantity = traded_quantity;

    trade.sequence = sequencer_.next();

    return trade;
}

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

    gateway.submit(&sell1);
    gateway.submit(&sell2);
    gateway.submit(&buy);

    std::cout << "sell1 remaining: " << sell1.quantity << '\n';
    std::cout << "sell2 remaining: " << sell2.quantity << '\n';
    std::cout << "buy remaining: " << buy.quantity << '\n';

    std::cout << "Buyer Position : "
          << position_manager.position(1001)
          << std::endl;

    std::cout << "Seller Position : "
          << position_manager.position(2001)
          << std::endl;

    return 0;
}