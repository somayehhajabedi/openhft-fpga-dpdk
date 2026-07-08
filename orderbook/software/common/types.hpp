#pragma once

#include <cstdint>

enum class Side
{
    Buy,
    Sell
};

using OrderId = std::uint64_t;
using Price   = std::int64_t;
using Quantity = std::uint32_t;
