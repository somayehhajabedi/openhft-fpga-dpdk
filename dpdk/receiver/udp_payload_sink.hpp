#pragma once

#include <cstdint>
#include <span>

class UdpPayloadSink
{
public:
    virtual ~UdpPayloadSink() = default;

    virtual bool submit(
        std::span<const std::uint8_t> payload) = 0;
};
