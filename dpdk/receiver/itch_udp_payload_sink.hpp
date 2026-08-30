#pragma once

#include "udp_payload_sink.hpp"
#include "market_data/replay/itch_replay_dispatcher.hpp"

#include <span>

class ItchUdpPayloadSink final : public UdpPayloadSink
{
public:
    explicit ItchUdpPayloadSink(
        ItchReplayDispatcher& dispatcher)
        :
        dispatcher_(dispatcher)
    {
    }

    bool submit(
        std::span<const std::uint8_t> payload) override
    {
        return dispatcher_.dispatch(
            payload.data(),
            payload.size());
    }

private:
    ItchReplayDispatcher& dispatcher_;
};
