#pragma once

#include <cstddef>
#include <cstdint>

namespace ouch
{

class OuchTransport
{
public:
    virtual ~OuchTransport() = default;

    virtual bool send(
        const std::uint8_t* data,
        std::size_t length) = 0;
};

} // namespace ouch
