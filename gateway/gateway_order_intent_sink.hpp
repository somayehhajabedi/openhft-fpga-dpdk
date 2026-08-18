#pragma once

#include "gateway/gateway.hpp"
#include "strategy/order_intent_sink.hpp"

class GatewayOrderIntentSink final
    : public OrderIntentSink
{
public:
    explicit GatewayOrderIntentSink(
        Gateway& gateway);

    bool submit(
        const OrderIntent& intent) override;

private:
    Gateway& gateway_;
};
