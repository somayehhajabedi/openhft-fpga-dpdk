#pragma once

#include <cstdint>
#include <string>

struct AddOrder
{
    std::uint16_t stockLocate;

    std::uint16_t trackingNumber;

    std::uint64_t orderReferenceNumber;

    bool isBuy;

    std::uint32_t shares;

    std::string symbol;

    std::uint32_t price;
};
