#pragma once

#include "gateway/order_execution_sink.hpp"
#include "risk/risk_manager.hpp"
#include "risk/risk_result.hpp"

class Gateway
{
public:
    Gateway(
        RiskManager& riskManager,
        OrderExecutionSink& executionSink);

    [[nodiscard]]
    RiskResult submit(
        const OrderIntent& intent);

private:
    RiskManager& riskManager_;
    OrderExecutionSink& executionSink_;
};
