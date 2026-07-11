#include "event_dispatcher.hpp"

#include "../market_data/trade_publisher.hpp"
#include "../journal/journal.hpp"

EventDispatcher::EventDispatcher(TradePublisher& trade_publisher,
                                 Journal& journal)
    : trade_publisher_(trade_publisher),
      journal_(journal)
{
}

void EventDispatcher::publish(const Trade& trade)
{
    journal_.write(trade);
    trade_publisher_.publish(trade);
}