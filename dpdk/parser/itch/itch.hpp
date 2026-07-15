#pragma once

#include <cstddef>
#include <cstdint>

struct ITCHHeader
{
    char message_type;
};

class ITCHParser
{
public:
    static const ITCHHeader* parse(
        const std::uint8_t* data,
        std::size_t length);

    static char messageType(
        const ITCHHeader* header);

    static bool isAddOrder(
        const ITCHHeader* header);

    static const std::uint8_t*
    payload(const ITCHHeader* header);
};