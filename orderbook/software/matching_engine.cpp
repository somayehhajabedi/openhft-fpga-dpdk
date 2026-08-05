#include <iostream>
#include "matching_engine.hpp"



MatchingEngine::MatchingEngine(EventDispatcher& dispatcher)
    : dispatcher_(dispatcher)
{
}



bool MatchingEngine::process(
    const MarketDataEvent& event)
{
    switch (event.type)
    {
        case MarketDataEventType::AddOrder:
            break;

        case MarketDataEventType::CancelOrder:
            break;

        case MarketDataEventType::DeleteOrder:
            break;

        case MarketDataEventType::ExecuteOrder:
            break;

        case MarketDataEventType::ReplaceOrder:
            break;
    }

    return false;
}

void MatchingEngine::process(
    Order* order)
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

