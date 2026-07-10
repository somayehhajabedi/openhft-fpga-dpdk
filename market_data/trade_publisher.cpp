#include "trade_publisher.hpp"

#include <iostream>

void TradePublisher::publish(const Trade& trade)
{
    std::cout << "TRADE: "
              << "BUY=" << trade.buy_order_id
              << " SELL=" << trade.sell_order_id
              << " PRICE=" << trade.price
              << " QTY=" << trade.quantity
              << '\n';
}