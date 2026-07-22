#include "gateway.hpp"

Gateway::Gateway(MatchingEngine& engine,
                 RiskManager& risk_manager)
    : engine_(engine),
      risk_manager_(risk_manager)
{
}


void Gateway::submit(Order* order)
{
    if (!validate(order))
        return;

    if (risk_manager_.check(order) != RiskResult::Accepted)
    {
        return;
    }

    engine_.process(order);
}

bool Gateway::validate(const Order* order) const
{
    if (!order)
        return false;

    if (order->quantity <= 0)
        return false;

    if (order->price <= 0)
        return false;

    return true;
}