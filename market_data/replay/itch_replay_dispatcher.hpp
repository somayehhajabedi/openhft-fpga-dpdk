#pragma once

#include "pipeline/market_data_event_sink.hpp"

#include <cstddef>
#include <cstdint>

class ItchReplayDispatcher
{
public:

    explicit ItchReplayDispatcher(
         MarketDataEventSink& sink);
    bool dispatch(
        const std::uint8_t* message,
        std::size_t length);

private:
    MarketDataEventSink& sink_;
};
