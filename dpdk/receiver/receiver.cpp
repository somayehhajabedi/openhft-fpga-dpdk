#include "receiver.hpp"

#include <iostream>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_lcore.h>

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