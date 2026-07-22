#pragma once

#include <cstddef>
#include <cstdint>

#include "order_cancel.hpp"

class OrderCancelParser
{
public:
    static const OrderCancelWireMessage* parse(
        const std::uint8_t* buffer,
        std::size_t length);

    static std::uint64_t orderReferenceNumber(
        const OrderCancelWireMessage* message);

    static std::uint32_t cancelledShares(
        const OrderCancelWireMessage* message);
};