

cmake_minimum_required(VERSION 3.20)

project(
    OpenHFTFPGADPDK
    LANGUAGES CXX
)

find_package(Threads REQUIRED)


include(cmake/Warnings.cmake)
include(cmake/Sanitizers.cmake) 
include(
    ${PROJECT_SOURCE_DIR}/cmake/Dependencies.cmake
)



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
  

add_subdirectory(dpdk/parser/ethernet)
add_subdirectory(dpdk/parser/ipv4)
add_subdirectory(dpdk/parser/udp)
add_subdirectory(dpdk/parser/itch)
add_subdirectory(pipeline)
add_subdirectory(strategy)
add_subdirectory(execution)
add_subdirectory(exchange)
add_subdirectory(market_data/replay)
add_subdirectory(common)
add_subdirectory(orderbook/software)
add_subdirectory(risk)
add_subdirectory(gateway)
add_subdirectory(apps)

add_subdirectory(benchmarks)

option(OPENHFT_BUILD_TESTS "Build unit and integration tests" ON)

if (OPENHFT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
