#pragma once

#include <cstdint>

struct OrderExecuted
{
    std::uint64_t orderReferenceNumber{};
    std::uint32_t executedShares{};
    std::uint64_t matchNumber{};
};