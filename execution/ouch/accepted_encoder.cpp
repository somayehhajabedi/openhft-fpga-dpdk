#include "execution/ouch/accepted_encoder.hpp"

#include "execution/ouch/ouch_wire_utils.hpp"

#include <algorithm>

namespace ouch
{

AcceptedEncoder::Buffer
AcceptedEncoder::encode(
    const Accepted& accepted)
{
    Buffer buffer{};

    //
    // Alpha fields are space padded.
    //
    buffer.fill(
        static_cast<std::uint8_t>(' '));

    //
    // Offset 0: Message Type
    //
    buffer[0] =
        static_cast<std::uint8_t>('A');

    //
    // Offset 1-8: Timestamp
    //
    wire::writeUint64(
        buffer.data() + 1,
        accepted.timestamp);

    //
    // Offset 9-12: UserRefNum
    //
    wire::writeUint32(
        buffer.data() + 9,
        accepted.userRefNum);

    //
    // Offset 13: Side
    //
    buffer[13] =
        static_cast<std::uint8_t>(
            accepted.side == Side::Buy
                ? 'B'
                : 'S');

    //
    // Offset 14-17: Quantity
    //
    wire::writeUint32(
        buffer.data() + 14,
        accepted.quantity);

    //
    // Offset 18-25: Symbol
    //
    std::copy(
        accepted.symbol.begin(),
        accepted.symbol.end(),
        buffer.begin() + 18);

    //
    // Offset 26-33: Price
    //
    wire::writeUint64(
        buffer.data() + 26,
        accepted.price);

    //
    // Offset 34: Time In Force
    //
    buffer[34] =
        static_cast<std::uint8_t>(
            accepted.timeInForce);

    //
    // Offset 35: Display
    //
    buffer[35] =
        static_cast<std::uint8_t>(
            accepted.display);

    //
    // Offset 36-43: Order Reference Number
    //
    wire::writeUint64(
        buffer.data() + 36,
        accepted.orderReferenceNumber);

    //
    // Offset 44: Capacity
    //
    buffer[44] =
        static_cast<std::uint8_t>(
            accepted.capacity);

    //
    // Offset 45: ISO Eligibility
    //
    buffer[45] =
        static_cast<std::uint8_t>(
            accepted.isoEligibility);

    //
    // Offset 46: Cross Type
    //
    buffer[46] =
        static_cast<std::uint8_t>(
            accepted.crossType);

    //
    // Offset 47: Order State
    //
    buffer[47] =
        static_cast<std::uint8_t>(
            accepted.orderState);

    //
    // Offset 48-61: ClOrdID
    //
    std::copy(
        accepted.clOrdId.begin(),
        accepted.clOrdId.end(),
        buffer.begin() + 48);

    //
    // Offset 62-63: Optional Appendage Length
    //
    wire::writeUint16(
        buffer.data() + 62,
        accepted.appendageLength);

    return buffer;
}

} // namespace ouch
