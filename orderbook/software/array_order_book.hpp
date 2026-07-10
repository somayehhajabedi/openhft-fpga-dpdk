#pragma once

#include "order.hpp"
#include "price_level.hpp"
#include "common/types.hpp"

#include <array>
#include <cstddef>
#include <unordered_map>

class ArrayOrderBook
{
public:
    static constexpr std::size_t MaxPriceLevels = 100000;

    void addOrder(Order* order);
    bool cancelOrder(OrderId id);

private:
    std::size_t priceToIndex(Price price) const;
    PriceLevel& getLevel(Side side, Price price);

private:
    std::array<PriceLevel, MaxPriceLevels> bid_levels_;
    std::array<PriceLevel, MaxPriceLevels> ask_levels_;

    std::unordered_map<OrderId, Order*> order_index_;
};
