#pragma once

#include <cstddef>
#include <cstdint>

namespace ouch::wire
{

inline std::uint16_t readUint16(
    const std::uint8_t* source)
{
    return
        (static_cast<std::uint16_t>(source[0]) << 8) |
        static_cast<std::uint16_t>(source[1]);
}

inline std::uint32_t readUint32(
    const std::uint8_t* source)
{
    return
        (static_cast<std::uint32_t>(source[0]) << 24) |
        (static_cast<std::uint32_t>(source[1]) << 16) |
        (static_cast<std::uint32_t>(source[2]) << 8) |
        static_cast<std::uint32_t>(source[3]);
}

inline std::uint64_t readUint64(
    const std::uint8_t* source)
{
    std::uint64_t value = 0;

    for (std::size_t index = 0;
         index < 8;
         ++index)
    {
        value =
            (value << 8) |
            static_cast<std::uint64_t>(
                source[index]);
    }

    return value;
}

} // namespace ouch::wire
