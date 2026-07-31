#include "order_pool.hpp"

OrderPool::OrderPool(std::size_t capacity)
    : storage_(capacity)
{
    free_list_.reserve(capacity);

    for (std::size_t i = 0; i < capacity; ++i)
    {
        free_list_.push_back(&storage_[i]);
    }
}

Order* OrderPool::acquire()
{
    if (free_list_.empty())
    {
        return nullptr;
    }

    Order* order = free_list_.back();
    free_list_.pop_back();

    return order;
}

void OrderPool::release(Order* order)
{
    if (order == nullptr)
    {
        return;
    }

    free_list_.push_back(order);
}

std::size_t OrderPool::capacity() const
{
    return storage_.size();
}

std::size_t OrderPool::available() const
{
    return free_list_.size();
}