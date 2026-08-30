#pragma once

#include <rte_ethdev.h>
#include <rte_mempool.h>

#include "config.hpp"
#include "udp_payload_sink.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

class Receiver
{
public:
    explicit Receiver(
        std::optional<std::size_t> rxCpu = std::nullopt);

    Receiver(
        UdpPayloadSink& payloadSink,
        std::optional<std::size_t> rxCpu = std::nullopt);

    bool initialize(
        int argc,
        char** argv);

    void run();

    void listPorts();
    void printPortInfo();

private:
    bool createMempool();

    bool configurePort(
        std::uint16_t portId);

    bool setupRxQueue(
        std::uint16_t portId);

    bool setupTxQueue(
        std::uint16_t portId);

    bool startPort(
        std::uint16_t portId);

    rte_mempool* mbuf_pool_ = nullptr;

    UdpPayloadSink* payloadSink_ = nullptr;

    std::optional<std::size_t> rxCpu_;
};
