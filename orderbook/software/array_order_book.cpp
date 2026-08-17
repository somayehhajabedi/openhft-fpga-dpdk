#include "array_order_book.hpp"

#include <bit>

namespace
{

std::uint64_t lowerBitsMask(
    std::size_t bitIndex)
{
    if (bitIndex == 0)
    {
        return 0;
    }

    if (bitIndex >= 64)
    {
        return ~std::uint64_t{0};
    }

    return
        (std::uint64_t{1} << bitIndex) - 1;
}

} // namespace


void ArrayOrderBook::addOrder(Order* order)
{
    PriceLevel& level =
        getLevel(
            order->side,
            order->price);

    if (level.empty())
    {
        level.price =
            order->price;

        if (order->side == Side::Buy)
        {
            setLevelActive(
                bid_level_bitmap_,
                order->price);
        }
        else
        {
            setLevelActive(
                ask_level_bitmap_,
                order->price);
        }
    }

    level.push_back(order);

    if (order->side == Side::Buy)
    {
        if (best_bid_ == 0 ||
            order->price > best_bid_)
        {
            best_bid_ =
                order->price;
        }
    }
    else
    {
        if (best_ask_ == 0 ||
            order->price < best_ask_)
        {
            best_ask_ =
                order->price;
        }
    }

    order_index_[order->id] =
        order;
}


Order* ArrayOrderBook::cancelOrder(
    OrderId id)
{
    auto it =
        order_index_.find(id);

    if (it == order_index_.end())
    {
        return nullptr;
    }

    Order* order =
        it->second;

    PriceLevel* level =
        order->level;

    level->remove(order);

    if (level->empty())
    {
        if (order->side == Side::Buy)
        {
            clearLevelActive(
                bid_level_bitmap_,
                order->price);
        }
        else
        {
            clearLevelActive(
                ask_level_bitmap_,
                order->price);
        }

        if (order->side == Side::Buy &&
            order->price == best_bid_)
        {
            refreshBestBid();
        }

        if (order->side == Side::Sell &&
            order->price == best_ask_)
        {
            refreshBestAsk();
        }
    }

    order_index_.erase(it);

    return order;
}


OrderUpdateResult ArrayOrderBook::reduceOrder(
    OrderId id,
    Quantity cancelledQuantity)
{
    OrderUpdateResult result;

    auto it =
        order_index_.find(id);

    if (it == order_index_.end())
    {
        return result;
    }

    Order* order =
        it->second;

    if (cancelledQuantity == 0 ||
        cancelledQuantity > order->quantity)
    {
        return result;
    }

    if (cancelledQuantity ==
        order->quantity)
    {
        Order* removedOrder =
            cancelOrder(id);

        if (removedOrder == nullptr)
        {
            return result;
        }

        result.success = true;
        result.removed_order =
            removedOrder;

        return result;
    }

    order->quantity -=
        cancelledQuantity;

    result.success = true;

    return result;
}


OrderUpdateResult ArrayOrderBook::executeOrder(
    OrderId id,
    Quantity executedQuantity)
{
    return reduceOrder(
        id,
        executedQuantity);
}


bool ArrayOrderBook::replaceOrder(
    OrderId originalOrderId,
    OrderId newOrderId,
    Quantity newQuantity,
    Price newPrice)
{
    if (newOrderId == 0 ||
        newQuantity == 0 ||
        newOrderId ==
            originalOrderId)
    {
        return false;
    }

    if (order_index_.contains(
            newOrderId))
    {
        return false;
    }

    auto it =
        order_index_.find(
            originalOrderId);

    if (it == order_index_.end())
    {
        return false;
    }

    const Side side =
        it->second->side;

    const AccountId accountId =
        it->second->account_id;

    Order* order =
        cancelOrder(
            originalOrderId);

    if (order == nullptr)
    {
        return false;
    }

    order->id =
        newOrderId;

    order->account_id =
        accountId;

    order->side =
        side;

    order->price =
        newPrice;

    order->quantity =
        newQuantity;

    order->level = nullptr;
    order->prev = nullptr;
    order->next = nullptr;

    addOrder(order);

    return true;
}


std::size_t ArrayOrderBook::priceToIndex(
    Price price) const
{
    return static_cast<std::size_t>(
        price);
}


PriceLevel& ArrayOrderBook::getLevel(
    Side side,
    Price price)
{
    const std::size_t index =
        priceToIndex(price);

    if (side == Side::Buy)
    {
        return bid_levels_[index];
    }

    return ask_levels_[index];
}


const PriceLevel* ArrayOrderBook::bestBid() const
{
    if (best_bid_ == 0)
    {
        return nullptr;
    }

    return &bid_levels_[
        priceToIndex(best_bid_)];
}


const PriceLevel* ArrayOrderBook::bestAsk() const
{
    if (best_ask_ == 0)
    {
        return nullptr;
    }

    return &ask_levels_[
        priceToIndex(best_ask_)];
}


void ArrayOrderBook::refreshBestBid()
{
    if (best_bid_ == 0)
    {
        return;
    }

    const std::size_t bestIndex =
        priceToIndex(best_bid_);

    std::size_t wordIndex =
        bestIndex /
        BitsPerBitmapWord;

    const std::size_t bitIndex =
        bestIndex %
        BitsPerBitmapWord;

    std::uint64_t word =
        bid_level_bitmap_[
            wordIndex];

    // Search only prices below
    // the removed best bid.
    word &=
        lowerBitsMask(
            bitIndex);

    while (word == 0)
    {
        if (wordIndex == 0)
        {
            best_bid_ = 0;
            return;
        }

        --wordIndex;

        word =
            bid_level_bitmap_[
                wordIndex];
    }

    const std::size_t
        highestSetBit =
            BitsPerBitmapWord -
            1 -
            static_cast<std::size_t>(
                std::countl_zero(
                    word));

    best_bid_ =
        static_cast<Price>(
            wordIndex *
                BitsPerBitmapWord +
            highestSetBit);
}


void ArrayOrderBook::refreshBestAsk()
{
    if (best_ask_ == 0)
    {
        return;
    }

    const std::size_t bestIndex =
        priceToIndex(best_ask_);

    std::size_t wordIndex =
        bestIndex /
        BitsPerBitmapWord;

    const std::size_t bitIndex =
        bestIndex %
        BitsPerBitmapWord;

    std::uint64_t word =
        ask_level_bitmap_[
            wordIndex];

    // Search only prices above
    // the removed best ask.
    word &=
        ~lowerBitsMask(
            bitIndex + 1);

    while (word == 0)
    {
        ++wordIndex;

        if (wordIndex >=
            BitmapWordCount)
        {
            best_ask_ = 0;
            return;
        }

        word =
            ask_level_bitmap_[
                wordIndex];
    }

    const std::size_t
        lowestSetBit =
            static_cast<std::size_t>(
                std::countr_zero(
                    word));

    best_ask_ =
        static_cast<Price>(
            wordIndex *
                BitsPerBitmapWord +
            lowestSetBit);
}


void ArrayOrderBook::setLevelActive(
    std::array<
        std::uint64_t,
        BitmapWordCount>& bitmap,
    Price price)
{
    const std::size_t index =
        priceToIndex(price);

    const std::size_t word =
        index /
        BitsPerBitmapWord;

    const std::size_t bit =
        index %
        BitsPerBitmapWord;

    bitmap[word] |=
        (std::uint64_t{1} << bit);
}


void ArrayOrderBook::clearLevelActive(
    std::array<
        std::uint64_t,
        BitmapWordCount>& bitmap,
    Price price)
{
    const std::size_t index =
        priceToIndex(price);

    const std::size_t word =
        index /
        BitsPerBitmapWord;

    const std::size_t bit =
        index %
        BitsPerBitmapWord;

    bitmap[word] &=
        ~(std::uint64_t{1} << bit);
}