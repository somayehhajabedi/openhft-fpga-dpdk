
/*
 * Dispatcher Implementation
 *
 * The dispatcher continuously removes MarketDataEvent objects
 * from the SPSC queue and forwards them to the configured
 * EventConsumer.
 *
 * Matching Engine integration will be added in a later
 * milestone.
 */

#include "pipeline/dispatcher.hpp"

Dispatcher::Dispatcher(
    EventQueue& queue,
    EventConsumer& consumer)
    :
    queue_(queue),
    consumer_(consumer)
{
}

std::size_t Dispatcher::dispatch()
{
    MarketDataEvent event;

    std::size_t dispatchedCount{0};

    while (queue_.tryPop(event))
    {
        consumer_.consume(event);

        ++dispatchedCount;
    }

    return dispatchedCount;
}

