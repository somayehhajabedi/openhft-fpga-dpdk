#include "itch_handler.hpp"
#include "dpdk/parser/itch/messages/add_order.hpp"
#include "dpdk/parser/itch/mapper/add_order_mapper.hpp"
#include "dpdk/parser/itch/messages/order_cancel_parser.hpp"
#include "dpdk/parser/itch/mapper/order_cancel_mapper.hpp"


ITCHHandler::ITCHHandler(
    ArrayOrderBook& orderBook)
    :
    orderBook_(orderBook)
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

    auto order = std::make_unique<Order>();

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

    orderBook_.addOrder(order.get());

    orders_.push_back(std::move(order));

    return true;
}

bool ITCHHandler::onOrderCancel(
    const std::uint8_t* payload,
    std::size_t length)
{
    const OrderCancelWireMessage* wire =
        OrderCancelParser::parse(payload, length);

    if (wire == nullptr)
        return false;

    const OrderCancel cancel =
        OrderCancelMapper::fromWire(wire);

    return orderBook_.reduceOrder(
        cancel.orderReferenceNumber,
        cancel.cancelledShares);
}