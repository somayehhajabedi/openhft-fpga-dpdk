#pragma once

#include "../orderbook/software/matching_engine.hpp"
#include "../orderbook/software/order.hpp"
#include "../risk/risk_manager.hpp"
#include "../risk/risk_result.hpp"

class Gateway
{
public:
    Gateway(MatchingEngine& engine, RiskManager& risk_manager);

    [[nodiscard]] RiskResult submit(Order* order);

private:
    MatchingEngine& engine_;
    RiskManager& risk_manager_;
};