#include "receiver.hpp"
#include "config.hpp"

#include "../parser/ethernet/ethernet.hpp"
#include "../parser/ipv4/ipv4.hpp"
#include "../parser/udp/udp.hpp"

#include <cstring>
#include <iostream>
#include <span>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_udp.h>


bool Receiver::initialize(
    int argc,
    char** argv)
{
    int ret =
        rte_eal_init(
            argc,
            argv);

    if (ret < 0)
    {
        return false;
    }

    return true;
}


void Receiver::run()
{
    std::cout
        << "Running receiver..."
        << std::endl;

    listPorts();

    printPortInfo();

    if (!createMempool())
    {
        return;
    }

    if (rte_eth_dev_count_avail() > 0)
    {
        if (!configurePort(0))
        {
            return;
        }

        if (!setupRxQueue(0))
        {
            return;
        }

        if (!setupTxQueue(0))
        {
            return;
        }

        if (!startPort(0))
        {
            return;
        }

        rte_mbuf* packets[BURST_SIZE];

        while (true)
        {
            const std::uint16_t received =
                rte_eth_rx_burst(
                    0,
                    0,
                    packets,
                    BURST_SIZE);

            if (received == 0)
            {
                continue;
            }

            std::cout
                << "Received "
                << received
                << " packets"
                << std::endl;

            for (std::uint16_t i = 0;
                 i < received;
                 ++i)
            {
                rte_mbuf* packet =
                    packets[i];

                const std::uint8_t* data =
                    rte_pktmbuf_mtod(
                        packet,
                        const std::uint8_t*);

                const std::size_t packetLength =
                    rte_pktmbuf_pkt_len(packet);

                const std::span<const std::uint8_t>
                    packetView{
                        data,
                        packetLength};

                const EthernetHeader* ethernet =
                    EthernetParser::parse(
                        packetView);

                if (!ethernet)
                {
                    rte_pktmbuf_free(packet);
                    continue;
                }

                if (EthernetParser::etherType(
                        ethernet) != 0x0800)
                {
                    rte_pktmbuf_free(packet);
                    continue;
                }

                const std::uint8_t* ipv4Data =
                    EthernetParser::payload(
                        ethernet);

                const std::size_t
                    ipv4AvailableLength =
                        packetLength -
                        sizeof(EthernetHeader);

                const std::span<
                    const std::uint8_t>
                    ipv4View{
                        ipv4Data,
                        ipv4AvailableLength};

                const IPv4Header* ipv4 =
                    IPv4Parser::parse(
                        ipv4View);

                if (!ipv4)
                {
                    rte_pktmbuf_free(packet);
                    continue;
                }

                if (ipv4->protocol != 17)
                {
                    rte_pktmbuf_free(packet);
                    continue;
                }

                const std::uint8_t* udpData =
                    IPv4Parser::payload(
                        ipv4);

                const std::size_t
                    udpAvailableLength =
                        IPv4Parser::payloadLength(
                            ipv4);

                const std::span<
                    const std::uint8_t>
                    udpView{
                        udpData,
                        udpAvailableLength};

                const UDPHeader* udp =
                    UDPParser::parse(
                        udpView);

                if (!udp)
                {
                    rte_pktmbuf_free(packet);
                    continue;
                }

                const std::uint8_t* payload =
                    UDPParser::payload(
                        udp);

                const std::uint16_t
                    payloadLength =
                        UDPParser::payloadLength(
                            udp);

                std::cout
                    << "Payload: ";

                std::cout.write(
                    reinterpret_cast<
                        const char*>(payload),
                    payloadLength);

                std::cout << '\n';

                rte_pktmbuf_free(packet);
            }
        }
    }
}


void Receiver::listPorts()
{
    std::uint16_t count =
        rte_eth_dev_count_avail();

    std::cout
        << "Available DPDK ports: "
        << count
        << std::endl;
}


void Receiver::printPortInfo()
{
    std::uint16_t portCount =
        rte_eth_dev_count_avail();

    if (portCount == 0)
    {
        std::cout
            << "No DPDK ports found."
            << std::endl;

        return;
    }

    rte_eth_dev_info info{};

    if (rte_eth_dev_info_get(
            0,
            &info) != 0)
    {
        std::cout
            << "Cannot read port info."
            << std::endl;

        return;
    }

    std::cout
        << "Driver : "
        << info.driver_name
        << std::endl;

    std::cout
        << "Max RX Queues : "
        << info.max_rx_queues
        << std::endl;

    std::cout
        << "Max TX Queues : "
        << info.max_tx_queues
        << std::endl;
}


bool Receiver::configurePort(
    std::uint16_t portId)
{
    rte_eth_conf portConf;

    std::memset(
        &portConf,
        0,
        sizeof(portConf));

    const int ret =
        rte_eth_dev_configure(
            portId,
            1,
            1,
            &portConf);

    if (ret < 0)
    {
        std::cout
            << "Failed to configure port."
            << std::endl;

        return false;
    }

    std::cout
        << "Port configured successfully."
        << std::endl;

    return true;
}


bool Receiver::createMempool()
{
    mbuf_pool_ =
        rte_pktmbuf_pool_create(
            "RECEIVER_MBUF_POOL",
            NUM_MBUFS,
            MBUF_CACHE_SIZE,
            0,
            RTE_MBUF_DEFAULT_BUF_SIZE,
            rte_socket_id());

    if (!mbuf_pool_)
    {
        std::cout
            << "Failed to create mbuf pool."
            << std::endl;

        return false;
    }

    std::cout
        << "Mbuf pool created successfully."
        << std::endl;

    return true;
}


bool Receiver::setupRxQueue(
    std::uint16_t portId)
{
    const int ret =
        rte_eth_rx_queue_setup(
            portId,
            0,
            RX_RING_SIZE,
            rte_eth_dev_socket_id(
                portId),
            nullptr,
            mbuf_pool_);

    if (ret < 0)
    {
        std::cout
            << "Failed to setup RX queue."
            << std::endl;

        return false;
    }

    std::cout
        << "RX queue created successfully."
        << std::endl;

    return true;
}


bool Receiver::setupTxQueue(
    std::uint16_t portId)
{
    const int ret =
        rte_eth_tx_queue_setup(
            portId,
            0,
            TX_RING_SIZE,
            rte_eth_dev_socket_id(
                portId),
            nullptr);

    if (ret < 0)
    {
        std::cout
            << "Failed to setup TX queue."
            << std::endl;

        return false;
    }

    std::cout
        << "TX queue created successfully."
        << std::endl;

    return true;
}


bool Receiver::startPort(
    std::uint16_t portId)
{
    const int ret =
        rte_eth_dev_start(
            portId);

    if (ret < 0)
    {
        std::cout
            << "Failed to start port "
            << portId
            << std::endl;

        return false;
    }

    std::cout
        << "Port "
        << portId
        << " started successfully."
        << std::endl;

    return true;
}