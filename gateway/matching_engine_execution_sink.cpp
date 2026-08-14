#include "gateway/matching_engine_execution_sink.hpp"

MatchingEngineExecutionSink::MatchingEngineExecutionSink(
    MatchingEngine& engine)
    :
    engine_(engine)
{
}

bool MatchingEngineExecutionSink::submit(
    const OrderIntent& intent)
{
    return engine_.submitOrder(
        intent.accountId,
        intent.side,
        intent.price,
        intent.quantity);
}
