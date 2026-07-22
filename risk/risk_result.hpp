#pragma once

enum class RiskResult
{
    Accepted,

    InvalidPrice,
    InvalidQuantity,

    MaxOrderQuantityExceeded,
    MaxOrderValueExceeded,

    MaxPositionExceeded
};