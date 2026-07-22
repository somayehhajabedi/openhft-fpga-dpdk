#pragma once

#include <cstdint>

constexpr uint16_t RX_RING_SIZE = 1024;
constexpr uint16_t TX_RING_SIZE = 1024;

constexpr uint16_t NUM_MBUFS = 8191;
constexpr uint16_t MBUF_CACHE_SIZE = 250;
constexpr uint16_t BURST_SIZE = 32;
