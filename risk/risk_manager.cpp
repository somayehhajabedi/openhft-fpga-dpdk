
#include "risk_manager.hpp"

#include <cstdlib>

RiskResult RiskManager::check(const Order* order) const
{
    if (order == nullptr)
        return RiskResult::InvalidQuantity;

    if (order->price <= 0)
        return RiskResult::InvalidPrice;

    if (order->quantity == 0)
        return RiskResult::InvalidQuantity;

    if (order->quantity > MaxOrderQuantity)
        return RiskResult::MaxOrderQuantityExceeded;

    const std::uint64_t orderValue =
        static_cast<std::uint64_t>(order->price) *
        static_cast<std::uint64_t>(order->quantity);

    if (orderValue > MaxOrderValue)
        return RiskResult::MaxOrderValueExceeded;

    auto it = positions_.find(order->account_id);

    std::int64_t currentPosition = 0;

    if (it != positions_.end())
        currentPosition = it->second;

    std::int64_t newPosition = currentPosition;

    if (order->side == Side::Buy)
        newPosition += static_cast<std::int64_t>(order->quantity);
    else
        newPosition -= static_cast<std::int64_t>(order->quantity);

    if (std::abs(newPosition) > MaxPosition)
        return RiskResult::MaxPositionExceeded;

    return RiskResult::Accepted;
}

void RiskManager::onAccepted(const Order* order)
{
    if (order == nullptr)
        return;

    auto& position = positions_[order->account_id];

    if (order->side == Side::Buy)
        position += static_cast<std::int64_t>(order->quantity);
    else
        position -= static_cast<std::int64_t>(order->quantity);
}