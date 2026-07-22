#pragma once

#include <cstddef>
#include <cstdint>

#include "order_replace.hpp"

class OrderReplaceParser
{
public:
    static const OrderReplaceWireMessage* parse(
        const std::uint8_t* buffer,
        std::size_t length);

    static std::uint64_t originalOrderReferenceNumber(
        const OrderReplaceWireMessage* message);

    static std::uint64_t newOrderReferenceNumber(
        const OrderReplaceWireMessage* message);

    static std::uint32_t shares(
        const OrderReplaceWireMessage* message);

    static std::uint32_t price(
        const OrderReplaceWireMessage* message);
};