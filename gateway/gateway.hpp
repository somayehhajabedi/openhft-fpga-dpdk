#pragma once

#include "../orderbook/software/matching_engine.hpp"
#include "../orderbook/software/order.hpp"
#include "../risk/risk_manager.hpp"

class Gateway
{
public:
    Gateway(MatchingEngine& engine, RiskManager& risk_manager);

    void submit(Order* order);

private:
    bool validate(const Order* order) const;

    MatchingEngine& engine_;
    RiskManager& risk_manager_;
};