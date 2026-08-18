#include "gateway/gateway_order_intent_sink.hpp"

GatewayOrderIntentSink::GatewayOrderIntentSink(
    Gateway& gateway)
    :
    gateway_(gateway)
{
}

bool GatewayOrderIntentSink::submit(
    const OrderIntent& intent)
{
    const GatewayResult result =
        gateway_.submit(intent);

    return result.accepted();
}
