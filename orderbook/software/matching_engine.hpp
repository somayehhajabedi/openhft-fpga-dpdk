#pragma once

#include "array_order_book.hpp"
#include "../../market_data/trade_publisher.hpp"
#include "order.hpp"
#include "trade.hpp"

class MatchingEngine
{
public:
    void process(Order* order);

private:
    bool canCross(const Order* order) const;
    void executeTrade(Order* incoming);
    bool matchOne(Order* incoming);
    Trade createTrade(const Order* incoming,
                  const Order* resting,
                  Quantity traded_quantity) const;
    ArrayOrderBook book_;
    TradePublisher publisher_;
};
