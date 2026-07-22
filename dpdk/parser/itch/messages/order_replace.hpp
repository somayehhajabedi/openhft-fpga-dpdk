#pragma once

#include <cstdint>

#pragma pack(push, 1)

struct OrderReplaceWireMessage
{
    char message_type;                         // Offset 0,  length 1
    std::uint16_t stock_locate;                // Offset 1,  length 2
    std::uint16_t tracking_number;             // Offset 3,  length 2
    std::uint8_t timestamp[6];                 // Offset 5,  length 6
    std::uint64_t original_order_reference;    // Offset 11, length 8
    std::uint64_t new_order_reference;         // Offset 19, length 8
    std::uint32_t shares;                      // Offset 27, length 4
    std::uint32_t price;                       // Offset 31, length 4
};

#pragma pack(pop)

static_assert(
    sizeof(OrderReplaceWireMessage) == 35,
    "OrderReplaceWireMessage must be exactly 35 bytes");