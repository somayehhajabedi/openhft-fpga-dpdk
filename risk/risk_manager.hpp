#pragma once

#include <cstdint>
#include <unordered_map>

#include "models/order_intent.hpp"
#include "risk_result.hpp"

class RiskManager
{
public:
    RiskResult check(
        const OrderIntent& intent) const;

    void onAccepted(
        const OrderIntent& intent);

private:
    static constexpr Quantity MaxOrderQuantity = 100000;
    static constexpr std::uint64_t MaxOrderValue = 10'000'000;
    static constexpr std::int64_t MaxPosition = 500;

    std::unordered_map<AccountId, std::int64_t> positions_;
};
