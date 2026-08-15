#include "execution/ouch/ouch_decoder.hpp"
#include "execution/ouch/ouch_wire_utils.hpp"

namespace ouch
{

std::optional<Executed>
OuchDecoder::decodeExecuted(
    const std::uint8_t* data,
    std::size_t length)
{
    static constexpr std::size_t
        ExecutedFixedSize = 36;

    if (data == nullptr ||
        length < ExecutedFixedSize)
    {
        return std::nullopt;
    }

    if (data[0] !=
        static_cast<std::uint8_t>('E'))
    {
        return std::nullopt;
    }

    Executed executed{};

    executed.timestamp =
        wire::readUint64(data + 1);

    executed.userRefNum =
        wire::readUint32(data + 9);

    executed.quantity =
        wire::readUint32(data + 13);

    executed.price =
        wire::readUint64(data + 17);

    executed.liquidityFlag =
        static_cast<char>(
            data[25]);

    executed.matchNumber =
        wire::readUint64(data + 26);

    executed.appendageLength =
        wire::readUint16(data + 34);

    const std::size_t expectedLength =
        ExecutedFixedSize +
        executed.appendageLength;

    if (length < expectedLength)
    {
        return std::nullopt;
    }

    return executed;
}

} // namespace ouch
