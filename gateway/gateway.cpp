#include "gateway.hpp"

Gateway::Gateway(
    RiskManager& riskManager,
    OrderExecutionSink& executionSink)
    :
    riskManager_(riskManager),
    executionSink_(executionSink)
{
}

RiskResult Gateway::submit(
    const OrderIntent& intent)
{
    const RiskResult result =
        riskManager_.check(intent);

    if (result != RiskResult::Accepted)
    {
        return result;
    }

    if (!executionSink_.submit(intent))
    {
        return result;
    }

    riskManager_.onAccepted(intent);

    return RiskResult::Accepted;
}
