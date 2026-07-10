#include <iostream>
#include "matching_engine.hpp"
#include "../../gateway/gateway.hpp"

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

    publisher_.publish(trade);

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
                                  Quantity traded_quantity) const
{
    Trade trade{};

    if (incoming->side == Side::Buy)
    {
        trade.buy_order_id = incoming->id;
        trade.sell_order_id = resting->id;
    }
    else
    {
        trade.buy_order_id = resting->id;
        trade.sell_order_id = incoming->id;
    }

    trade.price = resting->price;
    trade.quantity = traded_quantity;

    return trade;
}

int main()
{
    MatchingEngine engine;
    RiskManager risk_manager;
    Gateway gateway(engine, risk_manager);

    Order sell1{
        .id = 1,
        .side = Side::Sell,
        .price = 100,
        .quantity = 30
    };

    Order sell2{
        .id = 2,
        .side = Side::Sell,
        .price = 101,
        .quantity = 50
    };

    Order buy{
        .id = 3,
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

    return 0;
}