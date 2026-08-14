#pragma once

#include "models/order_intent.hpp"

class OrderIntentSink
{
public:
    virtual ~OrderIntentSink() = default;

    virtual bool submit(
        const OrderIntent& intent) = 0;
};
