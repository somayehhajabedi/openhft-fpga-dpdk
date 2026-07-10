#pragma once

#include "../orderbook/software/matching_engine.hpp"
#include "../orderbook/software/order.hpp"

class Gateway
{
public:
    explicit Gateway(MatchingEngine& engine);

    void submit(Order* order);

private:
    MatchingEngine& engine_;
    bool validate(const Order* order) const;
};