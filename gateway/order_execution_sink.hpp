#pragma once

#include "models/order_intent.hpp"

class OrderExecutionSink
{
public:
    virtual ~OrderExecutionSink() = default;

    virtual bool submit(
        const OrderIntent& intent) = 0;
};
