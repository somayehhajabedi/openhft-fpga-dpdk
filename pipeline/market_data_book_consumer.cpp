#include "pipeline/market_data_book_consumer.hpp"

MarketDataBookConsumer::MarketDataBookConsumer(
    ArrayOrderBook& book)
    :
    book_(book),
    orderPool_(DefaultOrderCapacity)
{
}

void MarketDataBookConsumer::consume(
    const MarketDataEvent& event)
{
    switch (event.type)
    {
        case MarketDataEventType::AddOrder:
        {
            Order* order = orderPool_.acquire();

            if (order == nullptr)
            {
                return;
            }

            order->id = event.orderId;
            order->account_id = event.accountId;
            order->side = event.side;
            order->price = event.price;
            order->quantity = event.quantity;

            order->level = nullptr;
            order->prev = nullptr;
            order->next = nullptr;

            book_.addOrder(order);

            break;
        }

        case MarketDataEventType::CancelOrder:
        {
            const OrderUpdateResult result =
                book_.reduceOrder(
                    event.orderId,
                    event.quantity);

            releaseIfOwned(
                result.removed_order);

            break;
        }

        case MarketDataEventType::DeleteOrder:
        {
            Order* removedOrder =
                book_.cancelOrder(
                    event.orderId);

            releaseIfOwned(
                removedOrder);

            break;
        }

        case MarketDataEventType::ExecuteOrder:
        {
            const OrderUpdateResult result =
                book_.executeOrder(
                    event.orderId,
                    event.quantity);

            releaseIfOwned(
                result.removed_order);

            break;
        }

        case MarketDataEventType::ReplaceOrder:
        {
            book_.replaceOrder(
                event.orderId,
                event.newOrderId,
                event.quantity,
                event.price);

            break;
        }
    }
}

void MarketDataBookConsumer::releaseIfOwned(
    Order* order)
{
    if (order != nullptr &&
        orderPool_.owns(order))
    {
        orderPool_.release(order);
    }
}
