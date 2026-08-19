#pragma once

#include "execution/ouch/ouch_messages.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace ouch
{

class ExecutedEncoder
{
public:
    static constexpr std::size_t ExecutedSize = 36;

    using Buffer =
        std::array<
            std::uint8_t,
            ExecutedSize>;

    [[nodiscard]]
    static Buffer encode(
        const Executed& executed);
};

} // namespace ouch
