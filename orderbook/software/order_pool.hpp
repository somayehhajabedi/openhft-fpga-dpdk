#pragma once

#include "order.hpp"

#include <cstddef>
#include <vector>

class OrderPool
{
public:
    explicit OrderPool(std::size_t capacity);

    Order* acquire();

    void release(Order* order);

    std::size_t capacity() const;

    std::size_t available() const;

private:
    std::vector<Order> storage_;
    std::vector<Order*> free_list_;
};