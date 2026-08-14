#include "risk_manager.hpp"

#include <cstdlib>

RiskResult RiskManager::check(
    const OrderIntent& intent) const
{
    if (intent.price <= 0)
    {
        return RiskResult::InvalidPrice;
    }

    if (intent.quantity == 0)
    {
        return RiskResult::InvalidQuantity;
    }

    if (intent.quantity > MaxOrderQuantity)
    {
        return RiskResult::MaxOrderQuantityExceeded;
    }

    const std::uint64_t orderValue =
        static_cast<std::uint64_t>(intent.price) *
        static_cast<std::uint64_t>(intent.quantity);

    if (orderValue > MaxOrderValue)
    {
        return RiskResult::MaxOrderValueExceeded;
    }

    auto it =
        positions_.find(intent.accountId);

    std::int64_t currentPosition = 0;

    if (it != positions_.end())
    {
        currentPosition = it->second;
    }

    std::int64_t newPosition =
        currentPosition;

    if (intent.side == Side::Buy)
    {
        newPosition +=
            static_cast<std::int64_t>(
                intent.quantity);
    }
    else
    {
        newPosition -=
            static_cast<std::int64_t>(
                intent.quantity);
    }

    if (std::abs(newPosition) > MaxPosition)
    {
        return RiskResult::MaxPositionExceeded;
    }

    return RiskResult::Accepted;
}

void RiskManager::onAccepted(
    const OrderIntent& intent)
{
    auto& position =
        positions_[intent.accountId];

    if (intent.side == Side::Buy)
    {
        position +=
            static_cast<std::int64_t>(
                intent.quantity);
    }
    else
    {
        position -=
            static_cast<std::int64_t>(
                intent.quantity);
    }
}
