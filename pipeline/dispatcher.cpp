#include "pipeline/dispatcher.hpp"

/*
 * Dispatcher Implementation
 *
 * The dispatcher continuously removes MarketDataEvent objects
 * from the SPSC queue.
 *
 * At this stage the dispatcher only validates the integration
 * between the queue and the pipeline.
 *
 * Matching Engine integration will be added later.
 */

Dispatcher::Dispatcher(
    EventQueue& queue,
    EventConsumer& consumer)
    :
    queue_(queue),
    consumer_(consumer)
{
}

void Dispatcher::dispatch()
{
    MarketDataEvent event;

    while (queue_.tryPop(event))
    {
        consumer_.consume(event);
    }
}

