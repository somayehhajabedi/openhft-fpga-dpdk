#pragma once

#include <rte_ethdev.h>
#include <rte_mempool.h>
#include "config.hpp"
#include <cstring>

class Receiver
{
public:
    bool initialize(int argc, char** argv);

    void run();
    void listPorts();
    void printPortInfo();
    

private:
    bool createMempool();

    rte_mempool* mbuf_pool_ = nullptr;
    
    bool configurePort(uint16_t port_id);

    bool setupRxQueue(uint16_t port_id);

    bool setupTxQueue(uint16_t port_id);

    bool startPort(uint16_t port_id);

};
