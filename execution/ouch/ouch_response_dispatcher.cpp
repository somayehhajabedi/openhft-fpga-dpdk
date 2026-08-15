#include "execution/ouch/ouch_response_dispatcher.hpp"

#include "execution/ouch/ouch_decoder.hpp"

namespace ouch
{

std::optional<OuchResponse>
OuchResponseDispatcher::dispatch(
    const std::uint8_t* data,
    std::size_t length)
{
    if (data == nullptr ||
        length == 0)
    {
        return std::nullopt;
    }

    switch (data[0])
    {
        case 'A':
        {
            const auto message =
                OuchDecoder::decodeAccepted(
                    data,
                    length);

            if (!message.has_value())
            {
                return std::nullopt;
            }

            return OuchResponse{
                *message};
        }

        case 'J':
        {
            const auto message =
                OuchDecoder::decodeRejected(
                    data,
                    length);

            if (!message.has_value())
            {
                return std::nullopt;
            }

            return OuchResponse{
                *message};
        }

        case 'E':
        {
            const auto message =
                OuchDecoder::decodeExecuted(
                    data,
                    length);

            if (!message.has_value())
            {
                return std::nullopt;
            }

            return OuchResponse{
                *message};
        }

        case 'C':
        {
            const auto message =
                OuchDecoder::decodeCanceled(
                    data,
                    length);

            if (!message.has_value())
            {
                return std::nullopt;
            }

            return OuchResponse{
                *message};
        }

        default:
            return std::nullopt;
    }
}

} // namespace ouch
