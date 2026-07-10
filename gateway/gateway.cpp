#include "gateway.hpp"

Gateway::Gateway(MatchingEngine& engine)
    : engine_(engine)
{
}

void Gateway::submit(Order* order)
{
    if (!validate(order))
        return;

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