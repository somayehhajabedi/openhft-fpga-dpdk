#pragma once

#include "execution/ouch/ouch_messages.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>

namespace ouch
{

using OuchResponse =
    std::variant<
        Accepted,
        Rejected,
        Executed,
        Canceled>;

class OuchResponseDispatcher
{
public:
    [[nodiscard]]
    static std::optional<OuchResponse> dispatch(
        const std::uint8_t* data,
        std::size_t length);
};

} // namespace ouch
