#pragma once

#include "orderbook/software/array_order_book.hpp"
#include "orderbook/software/order_pool.hpp"

#include "dpdk/parser/itch/messages/order_executed_parser.hpp"
#include "dpdk/parser/itch/mapper/order_executed_mapper.hpp"

#include <cstddef>
#include <cstdint>

class ITCHHandler
{
public:
    explicit ITCHHandler(
        ArrayOrderBook& orderBook);

    bool onAddOrder(
        const std::uint8_t* payload,
        std::size_t length);

    bool onOrderCancel(
        const std::uint8_t* payload,
        std::size_t length);

    bool onOrderDelete(
        const std::uint8_t* payload,
        std::size_t length);

    bool onOrderExecuted(
        const std::uint8_t* payload,
        std::size_t length);

    bool onOrderReplace(
        const std::uint8_t* payload,
        std::size_t length);

private:
    ArrayOrderBook& orderBook_;

    OrderPool orderPool_;
};