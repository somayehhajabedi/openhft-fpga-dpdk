#include <iostream>
#include "matching_engine.hpp"



MatchingEngine::MatchingEngine(
    EventDispatcher& dispatcher)
    :
    orderPool_(DefaultOrderCapacity),
    dispatcher_(dispatcher)
{
}



bool MatchingEngine::process(
    const MarketDataEvent& event)
{
    switch (event.type)
    {
        case MarketDataEventType::CancelOrder:
        {
            const OrderUpdateResult result =
                book_.reduceOrder(
                    event.orderId,
                    event.quantity);

            releaseIfOwned(result.removed_order);
            return result.success;
        }

        case MarketDataEventType::DeleteOrder:
        {
            Order* removedOrder =
                book_.cancelOrder(event.orderId);

            releaseIfOwned(removedOrder);
            return removedOrder != nullptr;
        }

        case MarketDataEventType::ExecuteOrder:
        {
            const OrderUpdateResult result =
                book_.executeOrder(
                    event.orderId,
                    event.quantity);

            releaseIfOwned(result.removed_order);
            return result.success;
        }

        case MarketDataEventType::AddOrder:
        {
            Order* order = orderPool_.acquire();

            if (order == nullptr)
            {
                return false;
            }

            order->id = event.orderId;
            order->account_id = event.accountId;
            order->side = event.side;
            order->price = event.price;
            order->quantity = event.quantity;

            order->level = nullptr;
            order->prev = nullptr;
            order->next = nullptr;

            process(order);

            return true;
        }


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

void MatchingEngine::executeTrade(
    Order* incoming)
{
    while (incoming->quantity > 0 && canCross(incoming))
    {
        if (!matchOne(incoming))
        {
            break;
        }
    }

    if (incoming->quantity > 0)
    {
        book_.addOrder(incoming);
    }
    else
    {
        releaseIfOwned(incoming);
    }
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
    {
        Order* removedOrder =
            book_.cancelOrder(resting->id);

        releaseIfOwned(removedOrder);
    }

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

void MatchingEngine::releaseIfOwned(
    Order* order)
{
    if (orderPool_.owns(order))
    {
        orderPool_.release(order);
    }
}

