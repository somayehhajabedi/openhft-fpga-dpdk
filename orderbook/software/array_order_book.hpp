#pragma once

#include "order.hpp"
#include "price_level.hpp"
#include "common/types.hpp"
#include "common/fixed_hash_map.hpp"
#include "strategy/market_view.hpp"

#include <array>
#include <cstddef>
#include <cstdint>


struct OrderUpdateResult
{
    bool success = false;
    Order* removed_order = nullptr;
};


class ArrayOrderBook final
    : public MarketView
{
public:
    static constexpr std::size_t MaxPriceLevels = 100000;
    static constexpr std::size_t DefaultOrderCapacity = 4096;

    void addOrder(Order* order);

    Order* cancelOrder(OrderId id);

    OrderUpdateResult reduceOrder(
        OrderId id,
        Quantity cancelledQuantity);

    OrderUpdateResult executeOrder(
        OrderId id,
        Quantity executedQuantity);

    bool replaceOrder(
        OrderId originalOrderId,
        OrderId newOrderId,
        Quantity newQuantity,
        Price newPrice);

    [[nodiscard]]
    const PriceLevel* bestBid() const override;

    [[nodiscard]]
    const PriceLevel* bestAsk() const override;


private:
    static constexpr std::size_t BitsPerBitmapWord = 64;

    static constexpr std::size_t BitmapWordCount =
        (MaxPriceLevels + BitsPerBitmapWord - 1) /
        BitsPerBitmapWord;

    std::size_t priceToIndex(
        Price price) const;

    PriceLevel& getLevel(
        Side side,
        Price price);

    void refreshBestBid();
    void refreshBestAsk();

    void setLevelActive(
        std::array<std::uint64_t, BitmapWordCount>& bitmap,
        Price price);

    void clearLevelActive(
        std::array<std::uint64_t, BitmapWordCount>& bitmap,
        Price price);


    std::array<PriceLevel, MaxPriceLevels> bid_levels_;
    std::array<PriceLevel, MaxPriceLevels> ask_levels_;

    std::array<
        std::uint64_t,
        BitmapWordCount> bid_level_bitmap_{};

    std::array<
        std::uint64_t,
        BitmapWordCount> ask_level_bitmap_{};

    Price best_bid_ = 0;
    Price best_ask_ = 0;

    FixedHashMap<
        OrderId,
        Order*,
        DefaultOrderCapacity> order_index_;
};