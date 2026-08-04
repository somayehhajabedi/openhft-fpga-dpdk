#include "pipeline/dispatcher.hpp"

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

Dispatcher::Dispatcher(
    EventQueue& queue,
    EventConsumer& consumer)
    :
    queue_(queue),
    consumer_(consumer)
{
}

bool Dispatcher::dispatch()
{
    MarketDataEvent event;

    bool dispatched{false};

    while (queue_.tryPop(event))
    {
        consumer_.consume(event);

        dispatched = true;
    }

    return dispatched;
}

