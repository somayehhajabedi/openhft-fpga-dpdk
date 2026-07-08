#pragma once

#include "order.hpp"

struct PriceLevel
{
    Price price;
    Order* head = nullptr;
    Order* tail = nullptr;
    Quantity total_quantity = 0;

    explicit PriceLevel(Price p)
        : price(p)
    {
    }

    bool empty() const
    {
        return head == nullptr;
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

        head = order->next;

        if (head)
            head->prev = nullptr;
        else
            tail = nullptr;

        total_quantity -= order->quantity;

        order->level = nullptr;
        order->prev = nullptr;
        order->next = nullptr;

        return order;
    }
};
