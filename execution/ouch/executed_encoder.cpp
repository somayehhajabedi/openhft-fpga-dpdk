#include "execution/ouch/executed_encoder.hpp"

#include "execution/ouch/ouch_wire_utils.hpp"

namespace ouch
{

ExecutedEncoder::Buffer
ExecutedEncoder::encode(
    const Executed& executed)
{
    Buffer buffer{};

    buffer[0] =
        static_cast<std::uint8_t>('E');

    wire::writeUint64(
        buffer.data() + 1,
        executed.timestamp);

    wire::writeUint32(
        buffer.data() + 9,
        executed.userRefNum);

    wire::writeUint32(
        buffer.data() + 13,
        executed.quantity);

    wire::writeUint64(
        buffer.data() + 17,
        executed.price);

    buffer[25] =
        static_cast<std::uint8_t>(
            executed.liquidityFlag);

    wire::writeUint64(
        buffer.data() + 26,
        executed.matchNumber);

    wire::writeUint16(
        buffer.data() + 34,
        executed.appendageLength);

    return buffer;
}

} // namespace ouch
