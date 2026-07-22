#include "risk_manager.hpp"

bool RiskManager::check(const Order* order) const
{
    if (!order)
        return false;

    if (order->price <= 0)
        return false;

    if (order->quantity <= 0)
        return false;

    if (order->quantity > MaxOrderQuantity)
        return false;

    return true;
}