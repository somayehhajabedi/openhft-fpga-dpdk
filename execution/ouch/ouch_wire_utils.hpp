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

inline void writeUint16(
    std::uint8_t* destination,
    std::uint16_t value)
{
    destination[0] =
        static_cast<std::uint8_t>(
            (value >> 8) & 0xFF);

    destination[1] =
        static_cast<std::uint8_t>(
            value & 0xFF);
}

inline void writeUint32(
    std::uint8_t* destination,
    std::uint32_t value)
{
    destination[0] =
        static_cast<std::uint8_t>(
            (value >> 24) & 0xFF);

    destination[1] =
        static_cast<std::uint8_t>(
            (value >> 16) & 0xFF);

    destination[2] =
        static_cast<std::uint8_t>(
            (value >> 8) & 0xFF);

    destination[3] =
        static_cast<std::uint8_t>(
            value & 0xFF);
}

inline void writeUint64(
    std::uint8_t* destination,
    std::uint64_t value)
{
    for (std::size_t index = 0;
         index < 8;
         ++index)
    {
        const std::size_t shift =
            (7 - index) * 8;

        destination[index] =
            static_cast<std::uint8_t>(
                (value >> shift) & 0xFF);
    }
}



} // namespace ouch::wire
