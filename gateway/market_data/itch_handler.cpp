#include "itch_handler.hpp"
#include "dpdk/parser/itch/messages/add_order.hpp"
#include "dpdk/parser/itch/mapper/add_order_mapper.hpp"
#include "dpdk/parser/itch/messages/order_cancel_parser.hpp"
#include "dpdk/parser/itch/mapper/order_cancel_mapper.hpp"
#include "dpdk/parser/itch/messages/order_delete_parser.hpp"
#include "dpdk/parser/itch/mapper/order_delete_mapper.hpp"
#include "dpdk/parser/itch/messages/order_replace_parser.hpp"
#include "dpdk/parser/itch/mapper/order_replace_mapper.hpp"


ITCHHandler::ITCHHandler(
    ArrayOrderBook& orderBook)
    :
    orderBook_(orderBook),
    orderPool_(4096)
{
}

bool ITCHHandler::onAddOrder(
    const std::uint8_t* payload,
    std::size_t length)
{
    const AddOrderWireMessage* wire =
        AddOrderParser::parse(payload, length);

    if (wire == nullptr)
    {
        return false;
    }

    const AddOrder add_order =
        AddOrderMapper::fromWire(wire);

    Order* order = orderPool_.acquire();

    if (order == nullptr)
    {
        return false;
    }

    order->id =
        add_order.orderReferenceNumber;

    order->account_id = 0;

    order->side =
        add_order.isBuy
            ? Side::Buy
            : Side::Sell;

    order->price =
        static_cast<Price>(add_order.price);

    order->quantity =
        static_cast<Quantity>(add_order.shares);

    order->level = nullptr;
    order->prev = nullptr;
    order->next = nullptr;

    orderBook_.addOrder(order);

    return true;
}

bool ITCHHandler::onOrderCancel(
    const std::uint8_t* payload,
    std::size_t length)
{
    const OrderCancelWireMessage* wire =
        OrderCancelParser::parse(payload, length);

    if (wire == nullptr)
    {
        return false;
    }

    const OrderCancel cancel =
        OrderCancelMapper::fromWire(wire);

    const OrderUpdateResult result =
        orderBook_.reduceOrder(
            cancel.orderReferenceNumber,
            cancel.cancelledShares);

    if (!result.success)
    {
        return false;
    }

    if (result.removed_order != nullptr)
    {
        orderPool_.release(result.removed_order);
    }

    return true;
}


bool ITCHHandler::onOrderDelete(
    const std::uint8_t* payload,
    std::size_t length)
{
    const OrderDeleteWireMessage* wire =
        OrderDeleteParser::parse(payload, length);

    if (wire == nullptr)
    {
        return false;
    }

    const OrderDelete orderDelete =
        OrderDeleteMapper::fromWire(wire);

    Order* order =
        orderBook_.cancelOrder(
            orderDelete.orderReferenceNumber);

    if (order == nullptr)
    {
        return false;
    }

    orderPool_.release(order);

    return true;
}

bool ITCHHandler::onOrderReplace(
    const std::uint8_t* payload,
    std::size_t length)
{
    const OrderReplaceWireMessage* wire =
        OrderReplaceParser::parse(payload, length);

    if (wire == nullptr)
        return false;

    const OrderReplace replacement =
        OrderReplaceMapper::fromWire(wire);

    return orderBook_.replaceOrder(
        replacement.originalOrderReferenceNumber,
        replacement.newOrderReferenceNumber,
        replacement.shares,
        replacement.price);
}

bool ITCHHandler::onOrderExecuted(
    const std::uint8_t* payload,
    std::size_t length)
{
    const OrderExecutedWireMessage* wire =
        OrderExecutedParser::parse(payload, length);

    if (wire == nullptr)
        return false;

    const OrderExecuted execution =
        OrderExecutedMapper::fromWire(wire);

    return orderBook_.executeOrder(
        execution.orderReferenceNumber,
        execution.executedShares);
}
