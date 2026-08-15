#pragma once

#include "execution/ouch/ouch_messages.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace ouch
{

class OuchEncoder
{
public:
    static constexpr std::size_t EnterOrderSize = 47;

    using Buffer =
        std::array<
            std::uint8_t,
            EnterOrderSize>;

    [[nodiscard]]
    static Buffer encode(
        const EnterOrder& order);
};

} // namespace ouch
