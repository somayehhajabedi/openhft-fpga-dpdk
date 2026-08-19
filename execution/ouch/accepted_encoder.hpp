#pragma once

#include "execution/ouch/ouch_messages.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace ouch
{

class AcceptedEncoder
{
public:
    static constexpr std::size_t AcceptedSize = 64;

    using Buffer =
        std::array<
            std::uint8_t,
            AcceptedSize>;

    [[nodiscard]]
    static Buffer encode(
        const Accepted& accepted);
};

} // namespace ouch
