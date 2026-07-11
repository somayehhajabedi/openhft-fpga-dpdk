#include "event_dispatcher.hpp"

void EventDispatcher::addListener(ITradeListener* listener)
{
    if (listener)
        listeners_.push_back(listener);
}

void EventDispatcher::publish(const Trade& trade)
{
    for (ITradeListener* listener : listeners_)
    {
        listener->onTrade(trade);
    }
}