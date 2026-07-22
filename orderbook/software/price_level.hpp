#pragma once

#include "common/types.hpp"
#include "order.hpp"

struct PriceLevel
{
    Price price = 0;

    Order* head = nullptr;
    Order* tail = nullptr;

    Quantity total_quantity = 0;

    PriceLevel() = default;

    explicit PriceLevel(Price p)
        : price(p)
    {
    }

    bool empty() const
    {
        return head == nullptr;
    }

    Order* front() const
    {
        return head;
    }

    void push_back(Order* order)
    {
        order->level = this;
        order->prev = tail;
        order->next = nullptr;

        if (tail)
            tail->next = order;
        else
            head = order;

        tail = order;
        total_quantity += order->quantity;
    }

    Order* pop_front()
    {
        Order* order = head;

        if (!order)
            return nullptr;

        remove(order);
        return order;
    }

    void remove(Order* order)
    {
        if (!order)
            return;

        if (order->prev)
            order->prev->next = order->next;
        else
            head = order->next;

        if (order->next)
            order->next->prev = order->prev;
        else
            tail = order->prev;

        total_quantity -= order->quantity;

        order->level = nullptr;
        order->prev = nullptr;
        order->next = nullptr;
    }
};
