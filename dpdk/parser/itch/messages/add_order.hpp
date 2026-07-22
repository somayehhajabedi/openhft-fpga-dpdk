#pragma once

#include <cstddef>
#include <cstdint>


#include "models/add_order.hpp"

#pragma pack(push, 1)

struct AddOrderWireMessage
{
    char message_type;                    // Offset 0, length 1
    std::uint16_t stock_locate;           // Offset 1, length 2
    std::uint16_t tracking_number;        // Offset 3, length 2
    std::uint8_t timestamp[6];             // Offset 5, length 6
    std::uint64_t order_reference_number; // Offset 11, length 8
    char buy_sell_indicator;              // Offset 19, length 1
    std::uint32_t shares;                 // Offset 20, length 4
    char stock[8];                         // Offset 24, length 8
    std::uint32_t price;                  // Offset 32, length 4
};

#pragma pack(pop)

static_assert(sizeof(AddOrderWireMessage) == 36);

class AddOrderParser
{
public:
    static const AddOrderWireMessage* parse(
        const std::uint8_t* data,
        std::size_t length);

    static char messageType(
        const AddOrderWireMessage* message);

    static std::uint16_t stockLocate(
        const AddOrderWireMessage* message);

    static std::uint16_t trackingNumber(
        const AddOrderWireMessage* message);

    static std::uint32_t shares(
        const AddOrderWireMessage* message);

    static std::uint32_t price(
        const AddOrderWireMessage* message);

     static std::uint64_t orderReferenceNumber(
    const AddOrderWireMessage* message);

static char buySellIndicator(
    const AddOrderWireMessage* message);
};