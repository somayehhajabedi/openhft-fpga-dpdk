#pragma once

#include <cstdint>

struct OrderReplace
{
    std::uint64_t originalOrderReferenceNumber{};
    std::uint64_t newOrderReferenceNumber{};
    std::uint32_t shares{};
    std::uint32_t price{};
};