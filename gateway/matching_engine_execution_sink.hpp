#pragma once

#include "gateway/order_execution_sink.hpp"
#include "orderbook/software/matching_engine.hpp"

class MatchingEngineExecutionSink final
    : public OrderExecutionSink
{
public:
    explicit MatchingEngineExecutionSink(
        MatchingEngine& engine);

    bool submit(
        const OrderIntent& intent) override;

private:
    MatchingEngine& engine_;
};
