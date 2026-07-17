#include "add_order.hpp"
#include <arpa/inet.h>
#include <endian.h>

const AddOrderWireMessage* AddOrderParser::parse(
    const std::uint8_t* data,
    std::size_t length)
{
    if (data == nullptr)
    {
        return nullptr;
    }

    if (length < sizeof(AddOrderWireMessage))
    {
        return nullptr;
    }

    return reinterpret_cast<const AddOrderWireMessage*>(data);
}

char AddOrderParser::messageType(
    const AddOrderWireMessage* message)
{
    return message ? message->message_type : '\0';
}

std::uint16_t AddOrderParser::stockLocate(
    const AddOrderWireMessage* message)
{
    return message ? ntohs(message->stock_locate) : 0;
}

std::uint16_t AddOrderParser::trackingNumber(
    const AddOrderWireMessage* message)
{
    return message ? ntohs(message->tracking_number) : 0;
}

std::uint32_t AddOrderParser::shares(
    const AddOrderWireMessage* message)
{
    return message ? ntohl(message->shares) : 0;
}

std::uint32_t AddOrderParser::price(
    const AddOrderWireMessage* message)
{
    return message ? ntohl(message->price) : 0;
}
/*
AddOrder AddOrderParser::toModel(
    const AddOrderWireMessage* message)
{
    AddOrder order{};

    order.stockLocate =
        stockLocate(message);

    order.trackingNumber =
        trackingNumber(message);

    order.orderReferenceNumber = 0; // TODO

    order.isBuy = false; // TODO

    order.shares =
        shares(message);

    order.price =
        price(message);

    return order;
}
    */

    std::uint64_t AddOrderParser::orderReferenceNumber(
    const AddOrderWireMessage* message)
{
    return message
        ? be64toh(message->order_reference_number)
        : 0;
}

char AddOrderParser::buySellIndicator(
    const AddOrderWireMessage* message)
{
    return message
        ? message->buy_sell_indicator
        : '\0';
}