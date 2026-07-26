#pragma once

#include "common/types.hpp"
#include "common/fixed_hash_map.hpp"
#include "order.hpp"
#include "price_level.hpp"

#include <array>
#include <cstddef>


class ArrayOrderBook
{
public:
    static constexpr std::size_t MaxPriceLevels = 100000;
    static constexpr std::size_t DefaultOrderCapacity = 4096;

    explicit ArrayOrderBook(
        std::size_t order_capacity = DefaultOrderCapacity);

    void addOrder(Order* order);

    bool cancelOrder(OrderId id);

    bool reduceOrder(
        OrderId id,
        Quantity cancelledQuantity);

    bool executeOrder(
        OrderId id,
        Quantity executedQuantity);

    bool replaceOrder(
        OrderId originalOrderId,
        OrderId newOrderId,
        Quantity newQuantity,
        Price newPrice);

    const PriceLevel* bestBid() const;
    const PriceLevel* bestAsk() const;

private:
    std::size_t priceToIndex(Price price) const;

    PriceLevel& getLevel(
        Side side,
        Price price);

    void refreshBestBid();
    void refreshBestAsk();

    std::array<PriceLevel, MaxPriceLevels> bid_levels_;
    std::array<PriceLevel, MaxPriceLevels> ask_levels_;

    Price best_bid_ = 0;
    Price best_ask_ = 0;

    FixedHashMap<
    OrderId,
    Order*,
    DefaultOrderCapacity> order_index_;
};