#include "execution/ouch/ouch_decoder.hpp"
#include "execution/ouch/ouch_wire_utils.hpp"

namespace ouch
{

std::optional<Canceled>
OuchDecoder::decodeCanceled(
    const std::uint8_t* data,
    std::size_t length)
{
    static constexpr std::size_t
        CanceledFixedSize = 18;

    if (data == nullptr ||
        length < CanceledFixedSize)
    {
        return std::nullopt;
    }

    if (data[0] !=
        static_cast<std::uint8_t>('C'))
    {
        return std::nullopt;
    }

    Canceled canceled{};

    canceled.timestamp =
        wire::readUint64(data + 1);

    canceled.userRefNum =
        wire::readUint32(data + 9);

    canceled.quantity =
        wire::readUint32(data + 13);

    canceled.reason =
        static_cast<char>(
            data[17]);

    canceled.appendageLength = 0;

    return canceled;
}

} // namespace ouch
