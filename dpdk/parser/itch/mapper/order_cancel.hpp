#pragma once

#include <cstdint>

struct OrderCancel
{
    std::uint64_t orderReferenceNumber{};
    std::uint32_t cancelledShares{};
};