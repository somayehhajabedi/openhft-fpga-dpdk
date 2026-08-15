#include "execution/ouch/ouch_decoder.hpp"
#include "execution/ouch/ouch_wire_utils.hpp"

#include <algorithm>

namespace ouch
{

std::optional<Accepted>
OuchDecoder::decodeAccepted(
    const std::uint8_t* data,
    std::size_t length)
{
    static constexpr std::size_t
        AcceptedFixedSize = 64;

    if (data == nullptr ||
        length < AcceptedFixedSize)
    {
        return std::nullopt;
    }

    if (data[0] !=
        static_cast<std::uint8_t>('A'))
    {
        return std::nullopt;
    }

    Accepted accepted{};

    accepted.timestamp =
        wire::readUint64(data + 1);

    accepted.userRefNum =
        wire::readUint32(data + 9);

    accepted.side =
        data[13] ==
                static_cast<std::uint8_t>('B')
            ? Side::Buy
            : Side::Sell;

    accepted.quantity =
        wire::readUint32(data + 14);

    std::copy_n(
        reinterpret_cast<const char*>(
            data + 18),
        accepted.symbol.size(),
        accepted.symbol.begin());

    accepted.price =
        wire::readUint64(data + 26);

    accepted.timeInForce =
        static_cast<TimeInForce>(
            data[34]);

    accepted.display =
        static_cast<Display>(
            data[35]);

    accepted.orderReferenceNumber =
        wire::readUint64(data + 36);

    accepted.capacity =
        static_cast<Capacity>(
            data[44]);

    accepted.isoEligibility =
        static_cast<IsoEligibility>(
            data[45]);

    accepted.crossType =
        static_cast<CrossType>(
            data[46]);

    accepted.orderState =
        static_cast<OrderState>(
            data[47]);

    std::copy_n(
        reinterpret_cast<const char*>(
            data + 48),
        accepted.clOrdId.size(),
        accepted.clOrdId.begin());

    accepted.appendageLength =
        wire::readUint16(data + 62);

    const std::size_t expectedLength =
        AcceptedFixedSize +
        accepted.appendageLength;

    if (length < expectedLength)
    {
        return std::nullopt;
    }

    return accepted;
}

} // namespace ouch
