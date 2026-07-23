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

    [[nodiscard]] bool empty() const
    {
        return head == nullptr;
    }

    [[nodiscard]] Order* front() const
    {
        return head;
    }

    void push_back(Order* order)
    {
        order->level = this;
        order->prev = tail;
        order->next = nullptr;

        if (tail != nullptr)
            tail->next = order;
        else
            head = order;

        tail = order;
        total_quantity += order->quantity;
    }

    Order* pop_front()
    {
        Order* order = head;

        if (order == nullptr)
            return nullptr;

        remove(order);
        return order;
    }

    void remove(Order* order)
    {
        if (!order)
            return;

        if (order->prev != nullptr)
            order->prev->next = order->next;
        else
            head = order->next;

        if (order->next != nullptr)
            order->next->prev = order->prev;
        else
            tail = order->prev;

        total_quantity -= order->quantity;

        order->level = nullptr;
        order->prev = nullptr;
        order->next = nullptr;
    }
};
