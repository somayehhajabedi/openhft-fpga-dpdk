#pragma once

#include <cstdint>
#include <array>
#include <cstdint>

enum class Side
{
    Buy,
    Sell
};

using OrderId = std::uint64_t;
using Price   = std::int64_t;
using Quantity = std::uint32_t;
using AccountId = std::uint64_t;
using Symbol = std::array<char, 8>;
