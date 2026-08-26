#pragma once

#include <bit>
#include <concepts>
#include <type_traits>

template <typename T>
concept Integral =
    std::is_integral_v<T>;

template <Integral T>
constexpr T fromBigEndian(
    T value) noexcept
{
    if constexpr (
        std::endian::native ==
        std::endian::little)
    {
        return std::byteswap(value);
    }

    return value;
}

template <Integral T>
constexpr T toBigEndian(
    T value) noexcept
{
    if constexpr (
        std::endian::native ==
        std::endian::little)
    {
        return std::byteswap(value);
    }

    return value;
}
