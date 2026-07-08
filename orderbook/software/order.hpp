#pragma once

#include "common/types.hpp"

struct PriceLevel;

struct Order
{
    OrderId id;
    Side side;
    Price price;
    Quantity quantity;

    PriceLevel* level = nullptr;
    Order* prev = nullptr;
    Order* next = nullptr;
};
