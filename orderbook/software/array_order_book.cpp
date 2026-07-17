#include "array_order_book.hpp"

void ArrayOrderBook::addOrder(Order* order)
{
    PriceLevel& level = getLevel(order->side, order->price);

    if (level.empty())
        level.price = order->price;

    level.push_back(order);
    if (order->side == Side::Buy)
    {
        if (best_bid_ == 0 || order->price > best_bid_)
           best_bid_ = order->price;
    }
    else
    {
       if (best_ask_ == 0 || order->price < best_ask_)
          best_ask_ = order->price;
    }

    order_index_[order->id] = order;
}

bool ArrayOrderBook::cancelOrder(OrderId id)
{
    auto it = order_index_.find(id);

    if (it == order_index_.end())
        return false;

    Order* order = it->second;
    PriceLevel* level = order->level;

    level->remove(order);

    if (level->empty())
{
    if (order->side == Side::Buy && order->price == best_bid_)
        refreshBestBid();

    if (order->side == Side::Sell && order->price == best_ask_)
        refreshBestAsk();
}

    order_index_.erase(it);

    return true;
}

bool ArrayOrderBook::reduceOrder(
    OrderId id,
    Quantity cancelledQuantity)
{
    auto it = order_index_.find(id);

    if (it == order_index_.end())
        return false;

    Order* order = it->second;

    if (cancelledQuantity == 0 ||
        cancelledQuantity > order->quantity)
    {
        return false;
    }

    if (cancelledQuantity == order->quantity)
    {
        return cancelOrder(id);
    }

    order->quantity -= cancelledQuantity;

    return true;
}

bool ArrayOrderBook::replaceOrder(
    OrderId originalOrderId,
    OrderId newOrderId,
    Quantity newQuantity,
    Price newPrice)
{
    auto it = order_index_.find(originalOrderId);

    if (it == order_index_.end())
        return false;

    if (newOrderId == 0 ||
        newQuantity == 0 ||
        newOrderId == originalOrderId)
    {
        return false;
    }

    if (order_index_.find(newOrderId) != order_index_.end())
        return false;

    Order* order = it->second;

    const Side side = order->side;
    const AccountId accountId = order->account_id;

    if (!cancelOrder(originalOrderId))
        return false;

    order->id = newOrderId;
    order->account_id = accountId;
    order->side = side;
    order->price = newPrice;
    order->quantity = newQuantity;

    order->level = nullptr;
    order->prev = nullptr;
    order->next = nullptr;

    addOrder(order);

    return true;
}

bool ArrayOrderBook::executeOrder(
    OrderId id,
    Quantity executedQuantity)
{
    return reduceOrder(id, executedQuantity);
}

std::size_t ArrayOrderBook::priceToIndex(Price price) const
{
    return static_cast<std::size_t>(price);
}

PriceLevel& ArrayOrderBook::getLevel(Side side, Price price)
{
    const std::size_t index = priceToIndex(price);

    if (side == Side::Buy)
        return bid_levels_[index];

    return ask_levels_[index];
}
const PriceLevel* ArrayOrderBook::bestBid() const
{
    if (best_bid_ == 0)
        return nullptr;

    return &bid_levels_[priceToIndex(best_bid_)];
}

const PriceLevel* ArrayOrderBook::bestAsk() const
{
    if (best_ask_ == 0)
        return nullptr;

    return &ask_levels_[priceToIndex(best_ask_)];
}

void ArrayOrderBook::refreshBestBid()
{
    while (best_bid_ > 0)
    {
        PriceLevel& level = bid_levels_[priceToIndex(best_bid_)];

        if (!level.empty())
            return;

        --best_bid_;
    }

    best_bid_ = 0;
}

void ArrayOrderBook::refreshBestAsk()
{
    while (best_ask_ > 0 && best_ask_ < MaxPriceLevels)
    {
        PriceLevel& level = ask_levels_[priceToIndex(best_ask_)];

        if (!level.empty())
            return;

        ++best_ask_;
    }

    best_ask_ = 0;
}
