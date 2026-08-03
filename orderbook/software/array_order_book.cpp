#include "array_order_book.hpp"

#include <bit>

namespace
{

// Returns a mask with all bits below bitIndex set.
//
// Examples:
// bitIndex = 0  -> 0b00000000
// bitIndex = 3  -> 0b00000111
// bitIndex = 64 -> 0xFFFFFFFFFFFFFFFF
//
// Used to ignore prices at or above a given price level during
// bitmap-based best bid/ask searches.

std::uint64_t lowerBitsMask(std::size_t bitIndex)
{
    if (bitIndex == 0)
        return 0;

    if (bitIndex >= 64)
        return ~std::uint64_t{0};

    return (std::uint64_t{1} << bitIndex) - 1;
}
}

ArrayOrderBook::ArrayOrderBook(
    std::size_t /*order_capacity*/)
{
}

void ArrayOrderBook::addOrder(Order* order)
{
    PriceLevel& level = getLevel(order->side, order->price);

    if (level.empty())
    {
        level.price = order->price;

        if (order->side == Side::Buy)
            setLevelActive(bid_level_bitmap_, order->price);
        else
            setLevelActive(ask_level_bitmap_, order->price);
    }

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

    order_index_.insert(order->id, order);
}


bool ArrayOrderBook::cancelOrder(OrderId id)
{
    Order** found = order_index_.find(id);

    if (found == nullptr)
        return false;

    Order* order = *found;
    PriceLevel* level = order->level;

    level->remove(order);

    if (level->empty())
    {
        if (order->side == Side::Buy)
            clearLevelActive(bid_level_bitmap_, order->price);
        else
            clearLevelActive(ask_level_bitmap_, order->price);

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

    order_index_.erase(id);

    return true;
}

bool ArrayOrderBook::reduceOrder(
    OrderId id,
    Quantity cancelledQuantity)
{
    Order** found = order_index_.find(id);

    if (found == nullptr)
        return false;

    Order* order = *found;

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
    Order** found = order_index_.find(originalOrderId);

    if (found == nullptr)
        return false;

    if (newOrderId == 0 ||
        newQuantity == 0 ||
        newOrderId == originalOrderId)
    {
        return false;
    }

    if (order_index_.contains(newOrderId))
        return false;

    Order* order = *found;

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

// Updates best_bid_ after the current best bid price level becomes empty.
//
// Instead of scanning every price level, this function searches the
// bid bitmap for the next highest active price level using bit operations.
// This significantly reduces the search cost compared to the previous
// linear implementation.
void ArrayOrderBook::refreshBestBid()
{
    if (best_bid_ == 0)
        return;

    std::size_t wordIndex =
        priceToIndex(best_bid_) / BitsPerBitmapWord;

    const std::size_t bitIndex =
        priceToIndex(best_bid_) % BitsPerBitmapWord;

    std::uint64_t word =
        bid_level_bitmap_[wordIndex];

    // The current best-bid level was removed.
    // Keep only prices lower than the old best bid.
    // Ignore the removed best bid level and all higher prices.
    word &= lowerBitsMask(bitIndex);

    // Continue searching lower bitmap words until an active price level
    // is found or the beginning of the bitmap is reached.
    while (word == 0)
    {
        if (wordIndex == 0)
        {
            best_bid_ = 0;
            return;
        }

        --wordIndex;
        word = bid_level_bitmap_[wordIndex];
    }

    // Find the highest active price within the bitmap word.
    const std::size_t highestSetBit =
        BitsPerBitmapWord -
        1 -
        static_cast<std::size_t>(
            std::countl_zero(word));

    best_bid_ = static_cast<Price>(
        wordIndex * BitsPerBitmapWord +
        highestSetBit);
}

// Updates best_ask_ after the current best ask price level becomes empty.
//
// Searches the ask bitmap for the next lowest active price level using
// bit operations instead of scanning the entire price range.
void ArrayOrderBook::refreshBestAsk()
{
    if (best_ask_ == 0)
        return;

    std::size_t wordIndex =
        priceToIndex(best_ask_) / BitsPerBitmapWord;

    const std::size_t bitIndex =
        priceToIndex(best_ask_) % BitsPerBitmapWord;

    std::uint64_t word =
        ask_level_bitmap_[wordIndex];

    // The current best ask was removed.
    // Ignore all prices below the removed level.
    // Ignore the removed best ask level and all lower prices.
    word &= ~lowerBitsMask(bitIndex + 1);

    // Continue searching higher bitmap words until an active price level
    // is found or the bitmap is exhausted.
    while (word == 0)
    {
        ++wordIndex;

        if (wordIndex >= BitmapWordCount)
        {
            best_ask_ = 0;
            return;
        }

        word = ask_level_bitmap_[wordIndex];
    }

    // Find the lowest active price within the bitmap word.
    const std::size_t lowestSetBit =
        static_cast<std::size_t>(std::countr_zero(word));

    best_ask_ = static_cast<Price>(
        wordIndex * BitsPerBitmapWord +
        lowestSetBit);
}

// Marks a price level as active in the bitmap.
//
// Called when the first order is inserted into an empty price level.
void ArrayOrderBook::setLevelActive(
    std::array<std::uint64_t, BitmapWordCount>& bitmap,
    Price price)
{
    const std::size_t index = priceToIndex(price);

    const std::size_t word = index / BitsPerBitmapWord;
    const std::size_t bit  = index % BitsPerBitmapWord;

    bitmap[word] |= (std::uint64_t{1} << bit);
}

// Clears a price level from the bitmap.
//
// Called when the last order is removed from a price level.
void ArrayOrderBook::clearLevelActive(
    std::array<std::uint64_t, BitmapWordCount>& bitmap,
    Price price)
{
    const std::size_t index = priceToIndex(price);

    const std::size_t word = index / BitsPerBitmapWord;
    const std::size_t bit  = index % BitsPerBitmapWord;

    bitmap[word] &= ~(std::uint64_t{1} << bit);
}

