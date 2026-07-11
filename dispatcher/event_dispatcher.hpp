#pragma once

#include "../events/i_trade_listener.hpp"

#include <vector>

class EventDispatcher
{
public:
    void addListener(ITradeListener* listener);

    void publish(const Trade& trade);

private:
    std::vector<ITradeListener*> listeners_;
};