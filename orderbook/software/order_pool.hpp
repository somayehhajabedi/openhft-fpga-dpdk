#pragma once

#include "order.hpp"

#include <cstddef>
#include <vector>
#include <cstdint>

class OrderPool
{
public:
    explicit OrderPool(std::size_t capacity);
    

    Order* acquire();

    void release(Order* order);

    std::size_t capacity() const;

    std::size_t available() const;

    bool owns(
    const Order* order) const;

private:
    std::vector<Order> storage_;
    std::vector<Order*> free_list_;
    std::vector<std::uint8_t> in_use_;
};