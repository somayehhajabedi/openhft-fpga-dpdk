#pragma once

#include <cstdint>

#include "orderbook/software/common/types.hpp"

struct AddOrder
{
    std::uint16_t stockLocate;

    std::uint16_t trackingNumber;

    std::uint64_t orderReferenceNumber;

    bool isBuy;

    std::uint32_t shares;

    Symbol symbol{};

    std::uint32_t price;
};
