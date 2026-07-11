#include "journal.hpp"

#include <iostream>

void Journal::write(const Trade& trade)
{
std::cout
    << "JOURNAL: "
    << "SEQ=" << trade.sequence
    << " BUY=" << trade.buy_order_id
    << " SELL=" << trade.sell_order_id
    << " PRICE=" << trade.price
    << " QTY=" << trade.quantity
    << '\n';
}