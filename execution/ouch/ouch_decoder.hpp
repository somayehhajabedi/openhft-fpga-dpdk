#pragma once

#include "execution/ouch/ouch_messages.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace ouch
{

class OuchDecoder
{
public:
    [[nodiscard]]
    static std::optional<EnterOrder> decodeEnterOrder(
        const std::uint8_t* data,
        std::size_t length);

    [[nodiscard]]
    static std::optional<Accepted> decodeAccepted(
        const std::uint8_t* data,
        std::size_t length);

    [[nodiscard]]
    static std::optional<Rejected> decodeRejected(
        const std::uint8_t* data,
        std::size_t length);

    [[nodiscard]]
    static std::optional<Executed> decodeExecuted(
        const std::uint8_t* data,
        std::size_t length);

    [[nodiscard]]
    static std::optional<Canceled> decodeCanceled(
        const std::uint8_t* data,
        std::size_t length);
};

} // namespace ouch
