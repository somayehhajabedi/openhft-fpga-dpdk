#pragma once

#include <cstddef>
#include <cstdint>

#include "order_executed.hpp"

class OrderExecutedParser
{
public:
    static const OrderExecutedWireMessage* parse(
        const std::uint8_t* buffer,
        std::size_t length);

    static std::uint64_t orderReferenceNumber(
        const OrderExecutedWireMessage* message);

    static std::uint32_t executedShares(
        const OrderExecutedWireMessage* message);

    static std::uint64_t matchNumber(
        const OrderExecutedWireMessage* message);
};