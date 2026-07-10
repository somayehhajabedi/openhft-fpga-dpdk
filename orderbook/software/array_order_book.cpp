#include "array_order_book.hpp"

void ArrayOrderBook::addOrder(Order* order)
{
    PriceLevel& level = getLevel(order->side, order->price);

    if (level.empty())
        level.price = order->price;

    level.push_back(order);

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
    order_index_.erase(it);

    return true;
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
