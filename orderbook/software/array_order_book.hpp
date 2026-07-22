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
    PriceLevel& getLevel(Side side, Price price);
    void refreshBestBid();
    void refreshBestAsk();

private:
    std::array<PriceLevel, MaxPriceLevels> bid_levels_;
    std::array<PriceLevel, MaxPriceLevels> ask_levels_;

    Price best_bid_ = 0;
    Price best_ask_ = 0;

    std::unordered_map<OrderId, Order*> order_index_;
};
