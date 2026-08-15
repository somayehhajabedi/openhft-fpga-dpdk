#include "execution/ouch/ouch_decoder.hpp"
#include "execution/ouch/ouch_wire_utils.hpp"

#include <algorithm>

namespace ouch
{

std::optional<Rejected>
OuchDecoder::decodeRejected(
    const std::uint8_t* data,
    std::size_t length)
{
    static constexpr std::size_t
        RejectedFixedSize = 31;

    if (data == nullptr ||
        length < RejectedFixedSize)
    {
        return std::nullopt;
    }

    if (data[0] !=
        static_cast<std::uint8_t>('J'))
    {
        return std::nullopt;
    }

    Rejected rejected{};

    rejected.timestamp =
        wire::readUint64(data + 1);

    rejected.userRefNum =
        wire::readUint32(data + 9);

    rejected.reason =
        static_cast<RejectReason>(
            wire::readUint16(data + 13));

    std::copy_n(
        reinterpret_cast<const char*>(
            data + 15),
        rejected.clOrdId.size(),
        rejected.clOrdId.begin());

    rejected.appendageLength =
        wire::readUint16(data + 29);

    const std::size_t expectedLength =
        RejectedFixedSize +
        rejected.appendageLength;

    if (length < expectedLength)
    {
        return std::nullopt;
    }

    return rejected;
}

} // namespace ouch
