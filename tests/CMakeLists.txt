cmake_minimum_required(VERSION 3.20)

project(OpenHFTTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

find_package(GTest REQUIRED)

get_filename_component(
    PROJECT_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    ABSOLUTE
)

add_executable(parser_tests
    unit/parser/ethernet_test.cpp
    unit/parser/ipv4_test.cpp
    unit/parser/udp_test.cpp
    unit/parser/itch_test.cpp
    unit/parser/add_order_test.cpp
    unit/mapper/add_order_mapper_test.cpp
    ../gateway/market_data/itch_handler.cpp
    ../orderbook/software/array_order_book.cpp
    


    integration/parser_pipeline_test.cpp
    integration/itch_handler_cancel_test.cpp

    ../dpdk/parser/ethernet/ethernet.cpp
    ../dpdk/parser/ipv4/ipv4.cpp
    ../dpdk/parser/udp/udp.cpp
    ../dpdk/parser/itch/itch.cpp
    ../dpdk/parser/itch/messages/add_order.cpp
    ../dpdk/parser/itch/mapper/add_order_mapper.cpp
    ../dpdk/parser/itch/messages/add_order.cpp
    ../dpdk/parser/itch/mapper/add_order_mapper.cpp
    ${PROJECT_ROOT}/dpdk/parser/itch/messages/order_cancel_parser.cpp
    ${PROJECT_ROOT}/dpdk/parser/itch/mapper/order_cancel_mapper.cpp
)

target_include_directories(parser_tests
    PRIVATE
        ${PROJECT_ROOT}
)

target_include_directories(parser_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(parser_tests PRIVATE
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(parser_tests)
