#pragma once

#include <cstddef>
#include <cstdint>

class ITCHHandler;

class ItchReplayDispatcher
{
public:
    explicit ItchReplayDispatcher(ITCHHandler& handler);

    bool dispatch(
        const std::uint8_t* message,
        std::size_t length);

private:
    ITCHHandler& handler_;
};