#pragma once

#include <cstdint>

#pragma pack(push, 1)

struct OrderExecutedWireMessage
{
    char message_type;                    // Offset 0,  length 1
    std::uint16_t stock_locate;           // Offset 1,  length 2
    std::uint16_t tracking_number;        // Offset 3,  length 2
    std::uint8_t timestamp[6];            // Offset 5,  length 6
    std::uint64_t order_reference_number; // Offset 11, length 8
    std::uint32_t executed_shares;        // Offset 19, length 4
    std::uint64_t match_number;            // Offset 23, length 8
};

#pragma pack(pop)

static_assert(
    sizeof(OrderExecutedWireMessage) == 31,
    "OrderExecutedWireMessage must be exactly 31 bytes");