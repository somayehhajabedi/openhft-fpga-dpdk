cmake_minimum_required(VERSION 3.20)

project(OpenHFTTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

find_package(GTest REQUIRED)

add_executable(parser_tests
    unit/parser/ethernet_test.cpp
    unit/parser/ipv4_test.cpp
    unit/parser/udp_test.cpp

    integration/parser_pipeline_test.cpp

    ../dpdk/parser/ethernet/ethernet.cpp
    ../dpdk/parser/ipv4/ipv4.cpp
    ../dpdk/parser/udp/udp.cpp
)

target_include_directories(parser_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(parser_tests PRIVATE
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(parser_tests)