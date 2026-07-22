#include "receiver.hpp"
#include "config.hpp"

#include "../parser/ethernet/ethernet.hpp"
#include "../parser/ipv4/ipv4.hpp"
#include "../parser/udp/udp.hpp"

#include <iostream>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_lcore.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <cstring>




bool Receiver::initialize(int argc, char** argv)
{
    int ret = rte_eal_init(argc, argv);

    if (ret < 0)
    {
        return false;
    }

    return true;
}

void Receiver::run()
{
    std::cout << "Running receiver..." << std::endl;

    listPorts();

    printPortInfo();

    if (!createMempool())
        return;

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
          const uint16_t received =
          rte_eth_rx_burst(
            0,
            0,
            packets,
            BURST_SIZE);

          if (received == 0)
            continue;

          std::cout << "Received "
              << received
              << " packets"
              << std::endl;

            for (std::uint16_t i = 0; i < received; ++i)
            {
                rte_mbuf* packet = packets[i];

                const std::uint8_t* data =
                    rte_pktmbuf_mtod(packet, const std::uint8_t*);

                const std::size_t packet_length =
                    rte_pktmbuf_pkt_len(packet);

                const EthernetHeader* ethernet =
                    EthernetParser::parse(data, packet_length);

                if (!ethernet)
                {
                    rte_pktmbuf_free(packet);
                    continue;
                }

                if (EthernetParser::etherType(ethernet) != 0x0800)
                {
                    rte_pktmbuf_free(packet);
                    continue;
                }

                const std::uint8_t* ipv4_data =
                    EthernetParser::payload(ethernet);

                const std::size_t ipv4_available_length =
                    packet_length - sizeof(EthernetHeader);

                const IPv4Header* ipv4 =
                    IPv4Parser::parse(
                        ipv4_data,
                        ipv4_available_length);

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

                const std::uint8_t* udp_data =
                    IPv4Parser::payload(ipv4);

                const std::size_t udp_available_length =
                    IPv4Parser::payloadLength(ipv4);

                const UDPHeader* udp =
                    UDPParser::parse(
                        udp_data,
                        udp_available_length);

                if (!udp)
                {
                    rte_pktmbuf_free(packet);
                    continue;
                }

                const std::uint8_t* payload =
                    UDPParser::payload(udp);

                const std::uint16_t payload_length =
                    UDPParser::payloadLength(udp);

                std::cout << "Payload: ";

                std::cout.write(
                    reinterpret_cast<const char*>(payload),
                    payload_length);

                std::cout << '\n';

                rte_pktmbuf_free(packet);
            }
        }
            
    }
        
}





void Receiver::listPorts()
{
    uint16_t count = rte_eth_dev_count_avail();

    std::cout << "Available DPDK ports: "
              << count
              << std::endl;
}

void Receiver::printPortInfo()
{
    uint16_t port_count = rte_eth_dev_count_avail();

    if (port_count == 0)
    {
        std::cout << "No DPDK ports found." << std::endl;
        return;
    }

    rte_eth_dev_info info{};

    if (rte_eth_dev_info_get(0, &info) != 0)
    {
        std::cout << "Cannot read port info." << std::endl;
        return;
    }

    std::cout << "Driver : " << info.driver_name << std::endl;
    std::cout << "Max RX Queues : " << info.max_rx_queues << std::endl;
    std::cout << "Max TX Queues : " << info.max_tx_queues << std::endl;
}

bool Receiver::configurePort(uint16_t port_id)
{
    rte_eth_conf port_conf;
    std::memset(&port_conf, 0, sizeof(port_conf));

    int ret = rte_eth_dev_configure(
        port_id,
        1,      // RX queues
        1,      // TX queues
        &port_conf);

    if (ret < 0)
    {
        std::cout << "Failed to configure port." << std::endl;
        return false;
    }

    std::cout << "Port configured successfully." << std::endl;

    return true;
}


bool Receiver::createMempool()
{
    mbuf_pool_ = rte_pktmbuf_pool_create(
        "RECEIVER_MBUF_POOL",
        NUM_MBUFS,
        MBUF_CACHE_SIZE,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id());

    if (!mbuf_pool_)
    {
        std::cout << "Failed to create mbuf pool."
                  << std::endl;
        return false;
    }

    std::cout << "Mbuf pool created successfully."
              << std::endl;

    return true;
}

bool Receiver::setupRxQueue(uint16_t port_id)
{
    constexpr uint16_t RX_RING_SIZE = 1024;

    int ret = rte_eth_rx_queue_setup(
        port_id,
        0,
        RX_RING_SIZE,
        rte_eth_dev_socket_id(port_id),
        nullptr,
        mbuf_pool_);

    if (ret < 0)
    {
        std::cout << "Failed to setup RX queue." << std::endl;
        return false;
    }

    std::cout << "RX queue created successfully." << std::endl;

    return true;
}

bool Receiver::setupTxQueue(uint16_t port_id)
{
    const int ret = rte_eth_tx_queue_setup(
        port_id,
        0,                          // queue id
        TX_RING_SIZE,
        rte_eth_dev_socket_id(port_id),
        nullptr);

    if (ret < 0)
    {
        std::cout << "Failed to setup TX queue." << std::endl;
        return false;
    }

    std::cout << "TX queue created successfully." << std::endl;

    return true;
}

bool Receiver::startPort(uint16_t port_id)
{
    const int ret = rte_eth_dev_start(port_id);

    if (ret < 0)
    {
        std::cout << "Failed to start port "
                  << port_id
                  << std::endl;

        return false;
    }

    std::cout << "Port "
              << port_id
              << " started successfully."
              << std::endl;

    return true;
}