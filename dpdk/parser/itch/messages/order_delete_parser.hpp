#pragma once

#include <cstddef>
#include <cstdint>

#include "order_delete.hpp"

class OrderDeleteParser
{
public:
    static const OrderDeleteWireMessage* parse(
        const std::uint8_t* buffer,
        std::size_t length);

    static std::uint64_t orderReferenceNumber(
        const OrderDeleteWireMessage* message);
};