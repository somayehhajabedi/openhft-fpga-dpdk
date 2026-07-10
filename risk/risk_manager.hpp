#pragma once

#include <unordered_map>
#include "../orderbook/software/order.hpp"

class RiskManager
{
public:
    bool check(const Order* order) const;
    void onAccepted(const Order* order);

private:
    static constexpr Quantity MaxOrderQuantity = 100000;
    static constexpr Quantity MaxPosition = 500;

    std::unordered_map<Side, Quantity> positions_;

};