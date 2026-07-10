#include "gateway.hpp"

Gateway::Gateway(MatchingEngine& engine)
    : engine_(engine)
{
}

void Gateway::submit(Order* order)
{
    engine_.process(order);
}