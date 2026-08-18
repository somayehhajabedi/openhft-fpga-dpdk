#pragma once

#include "risk/risk_result.hpp"

struct GatewayResult
{
    RiskResult riskResult{
        RiskResult::Accepted};

    bool executionSucceeded{false};

    [[nodiscard]]
    bool accepted() const noexcept
    {
        return
            riskResult == RiskResult::Accepted &&
            executionSucceeded;
    }
};
