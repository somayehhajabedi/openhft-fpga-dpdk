cmake_minimum_required(VERSION 3.20)

project(
    OpenHFT
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
  

add_subdirectory(dpdk/parser/ethernet)
add_subdirectory(dpdk/parser/ipv4)
add_subdirectory(dpdk/parser/udp)
add_subdirectory(dpdk/parser/itch)
add_subdirectory(market_data/replay)
add_subdirectory(gateway)
add_subdirectory(orderbook/software)

option(OPENHFT_BUILD_TESTS "Build unit and integration tests" ON)

if (OPENHFT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
