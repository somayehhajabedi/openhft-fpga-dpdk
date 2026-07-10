#pragma once

#include "array_order_book.hpp"
#include "order.hpp"

class MatchingEngine
{
public:
    void process(Order* order);

private:
    bool canCross(const Order* order) const;
    void executeTrade(Order* incoming);
    bool matchOne(Order* incoming);

private:
    ArrayOrderBook book_;
};
