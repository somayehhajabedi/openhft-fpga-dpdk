#pragma once

#include <cstdint>
#include <unordered_map>

#include "../orderbook/software/order.hpp"
#include "risk_result.hpp"

class RiskManager
{
public:
    RiskResult check(const Order* order) const;

    void onAccepted(const Order* order);

private:
    static constexpr Quantity MaxOrderQuantity = 100000;
    static constexpr std::uint64_t MaxOrderValue = 10'000'000;
    static constexpr std::int64_t MaxPosition = 500;

    std::unordered_map<AccountId, std::int64_t> positions_;
};