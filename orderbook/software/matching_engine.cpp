#include "matching_engine.hpp"

MatchingEngine::MatchingEngine(
    EventDispatcher& dispatcher)
    :
    orderPool_(DefaultOrderCapacity),
    dispatcher_(dispatcher)
{
}


bool MatchingEngine::submitOrder(
    AccountId accountId,
    Side side,
    Price price,
    Quantity quantity)
{
    Order* order =
        orderPool_.acquire();

    if (order == nullptr)
    {
        return false;
    }

    order->id =
        sequencer_.next();

    order->account_id =
        accountId;

    order->side =
        side;

    order->price =
        price;

    order->quantity =
        quantity;

    order->level = nullptr;
    order->prev = nullptr;
    order->next = nullptr;

    process(order);

    return true;
}

void MatchingEngine::process(
    Order* order)
{
    if (order == nullptr)
    {
        return;
    }

    if (canCross(order))
    {
        executeTrade(order);
    }
    else
    {
        book_.addOrder(order);
    }
}

bool MatchingEngine::canCross(
    const Order* order) const
{
    if (order->side == Side::Buy)
    {
        const PriceLevel* bestAsk =
            book_.bestAsk();

        return bestAsk != nullptr &&
               order->price >= bestAsk->price;
    }

    const PriceLevel* bestBid =
        book_.bestBid();

    return bestBid != nullptr &&
           order->price <= bestBid->price;
}

void MatchingEngine::executeTrade(
    Order* incoming)
{
    while (
        incoming->quantity > 0 &&
        canCross(incoming))
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

bool MatchingEngine::matchOne(
    Order* incoming)
{
    const PriceLevel* oppositeLevel =
        incoming->side == Side::Buy
            ? book_.bestAsk()
            : book_.bestBid();

    if (oppositeLevel == nullptr ||
        oppositeLevel->front() == nullptr)
    {
        return false;
    }

    Order* resting =
        oppositeLevel->front();

    const Quantity tradedQuantity =
        incoming->quantity < resting->quantity
            ? incoming->quantity
            : resting->quantity;

    const Trade trade =
        createTrade(
            incoming,
            resting,
            tradedQuantity);

    dispatcher_.publish(trade);

    incoming->quantity -=
        tradedQuantity;

    resting->quantity -=
        tradedQuantity;

    if (resting->level != nullptr)
    {
        resting->level->total_quantity -=
            tradedQuantity;
    }

    if (resting->quantity == 0)
    {
        Order* removedOrder =
            book_.cancelOrder(
                resting->id);

        releaseIfOwned(
            removedOrder);
    }

    return tradedQuantity > 0;
}

Trade MatchingEngine::createTrade(
    const Order* incoming,
    const Order* resting,
    Quantity tradedQuantity)
{
    Trade trade{};

    if (incoming->side == Side::Buy)
    {
        trade.buy_order_id =
            incoming->id;

        trade.sell_order_id =
            resting->id;

        trade.buy_account_id =
            incoming->account_id;

        trade.sell_account_id =
            resting->account_id;
    }
    else
    {
        trade.buy_order_id =
            resting->id;

        trade.sell_order_id =
            incoming->id;

        trade.buy_account_id =
            resting->account_id;

        trade.sell_account_id =
            incoming->account_id;
    }

    trade.price =
        resting->price;

    trade.quantity =
        tradedQuantity;

    trade.sequence =
        sequencer_.next();

    return trade;
}

void MatchingEngine::releaseIfOwned(
    Order* order)
{
    if (order != nullptr &&
        orderPool_.owns(order))
    {
        orderPool_.release(order);
    }
}
