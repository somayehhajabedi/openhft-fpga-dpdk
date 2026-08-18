#include "execution/ouch/ouch_decoder.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace
{

std::uint16_t readUint16(
    const std::uint8_t* source)
{
    return
        (static_cast<std::uint16_t>(source[0]) << 8) |
        static_cast<std::uint16_t>(source[1]);
}

std::uint32_t readUint32(
    const std::uint8_t* source)
{
    return
        (static_cast<std::uint32_t>(source[0]) << 24) |
        (static_cast<std::uint32_t>(source[1]) << 16) |
        (static_cast<std::uint32_t>(source[2]) << 8) |
        static_cast<std::uint32_t>(source[3]);
}

std::uint64_t readUint64(
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

} // namespace

namespace ouch
{

std::optional<EnterOrder>
OuchDecoder::decodeEnterOrder(
    const std::uint8_t* data,
    std::size_t length)
{
    if (data == nullptr ||
        length < 47)
    {
        return std::nullopt;
    }

    if (data[0] !=
        static_cast<std::uint8_t>('O'))
    {
        return std::nullopt;
    }

    EnterOrder order{};

    order.userRefNum =
        readUint32(data + 1);

    order.side =
        data[5] == static_cast<std::uint8_t>('B')
            ? Side::Buy
            : Side::Sell;

    order.quantity =
        readUint32(data + 6);

    std::copy_n(
        data + 10,
        order.symbol.size(),
        order.symbol.begin());

    order.price =
        readUint64(data + 18);

    order.timeInForce =
        static_cast<TimeInForce>(
            data[26]);

    order.display =
        static_cast<Display>(
            data[27]);

    order.capacity =
        static_cast<Capacity>(
            data[28]);

    order.isoEligibility =
        static_cast<IsoEligibility>(
            data[29]);

    order.crossType =
        static_cast<CrossType>(
            data[30]);

    std::copy_n(
        data + 31,
        order.clOrdId.size(),
        order.clOrdId.begin());

    order.appendageLength =
        readUint16(data + 45);

    return order;
}

} // namespace ouch
