#include "execution/ouch/ouch_encoder.hpp"

#include <algorithm>

namespace
{

void writeUint16(
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

void writeUint32(
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

void writeUint64(
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

} // namespace

namespace ouch
{

OuchEncoder::Buffer OuchEncoder::encode(
    const EnterOrder& order)
{
    Buffer buffer{};

    // Alpha fields in OUCH are space padded.
    buffer.fill(
        static_cast<std::uint8_t>(' '));

    // Offset 0: Message Type
    buffer[0] =
        static_cast<std::uint8_t>('O');

    // Offset 1-4: UserRefNum
    writeUint32(
        buffer.data() + 1,
        order.userRefNum);

    // Offset 5: Side
    buffer[5] =
        static_cast<std::uint8_t>(
            order.side == Side::Buy
                ? 'B'
                : 'S');

    // Offset 6-9: Quantity
    writeUint32(
        buffer.data() + 6,
        order.quantity);

    // Offset 10-17: Symbol
    std::copy(
        order.symbol.begin(),
        order.symbol.end(),
        buffer.begin() + 10);

    // Offset 18-25: Price
    writeUint64(
        buffer.data() + 18,
        order.price);

    // Offset 26: Time In Force
    buffer[26] =
        static_cast<std::uint8_t>(
            order.timeInForce);

    // Offset 27: Display
    buffer[27] =
        static_cast<std::uint8_t>(
            order.display);

    // Offset 28: Capacity
    buffer[28] =
        static_cast<std::uint8_t>(
            order.capacity);

    // Offset 29: InterMarket Sweep Eligibility
    buffer[29] =
        static_cast<std::uint8_t>(
            order.isoEligibility);

    // Offset 30: Cross Type
    buffer[30] =
        static_cast<std::uint8_t>(
            order.crossType);

    // Offset 31-44: ClOrdID
    std::copy(
        order.clOrdId.begin(),
        order.clOrdId.end(),
        buffer.begin() + 31);

    // Offset 45-46: Optional Appendage Length.
    // Optional appendages are not supported yet.
    writeUint16(
        buffer.data() + 45,
        0);

    return buffer;
}

} // namespace ouch
