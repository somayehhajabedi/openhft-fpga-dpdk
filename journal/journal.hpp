#pragma once

#include "../orderbook/software/trade.hpp"

class Journal
{
public:
    void write(const Trade& trade);
};