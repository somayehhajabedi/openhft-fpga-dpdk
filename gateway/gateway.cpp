#include "gateway.hpp"

Gateway::Gateway(
    RiskManager& riskManager,
    OrderExecutionSink& executionSink)
    :
    riskManager_(riskManager),
    executionSink_(executionSink)
{
}

GatewayResult Gateway::submit(
    const OrderIntent& intent)
{
    const RiskResult riskResult =
        riskManager_.check(intent);

    if (riskResult != RiskResult::Accepted)
    {
        return GatewayResult{
            .riskResult = riskResult,
            .executionSucceeded = false
        };
    }

    if (!executionSink_.submit(intent))
    {
        return GatewayResult{
            .riskResult = RiskResult::Accepted,
            .executionSucceeded = false
        };
    }

    riskManager_.onAccepted(intent);

    return GatewayResult{
        .riskResult = RiskResult::Accepted,
        .executionSucceeded = true
    };
}
