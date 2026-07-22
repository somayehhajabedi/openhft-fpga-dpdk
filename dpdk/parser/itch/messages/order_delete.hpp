#pragma once

#include <cstdint>

#pragma pack(push, 1)

struct OrderDeleteWireMessage
{
    char message_type;
    std::uint16_t stock_locate;
    std::uint16_t tracking_number;
    std::uint8_t timestamp[6];
    std::uint64_t order_reference_number;
};

#pragma pack(pop)

static_assert(
    sizeof(OrderDeleteWireMessage) == 19,
    "OrderDeleteWireMessage must be exactly 19 bytes");