#pragma once

#include "orderbook/software/array_order_book.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class ITCHHandler
{
public:

    explicit ITCHHandler(ArrayOrderBook& orderBook);

    bool onAddOrder(
        const std::uint8_t* payload,
        std::size_t length);
    bool onOrderCancel(
        const std::uint8_t* payload,
        std::size_t length);

private:

    ArrayOrderBook& orderBook_;

    std::vector<std::unique_ptr<Order>> orders_;
};