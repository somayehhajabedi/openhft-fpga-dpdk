
#include "gateway.hpp"

Gateway::Gateway(MatchingEngine& engine,
                 RiskManager& risk_manager)
    : engine_(engine),
      risk_manager_(risk_manager)
{
}

RiskResult Gateway::submit(Order* order)
{
    const RiskResult result = risk_manager_.check(order);

    if (result != RiskResult::Accepted)
        return result;

    risk_manager_.onAccepted(order);
    engine_.process(order);

    return RiskResult::Accepted;
}