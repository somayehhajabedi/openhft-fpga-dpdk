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


    unit/parser/order_delete_parser_test.cpp
    unit/mapper/order_delete_mapper_test.cpp
    unit/parser/order_executed_parser_test.cpp
    unit/mapper/order_executed_mapper_test.cpp
    unit/parser/order_replace_parser_test.cpp
    unit/mapper/order_replace_mapper_test.cpp
    unit/replay/itch_replay_reader_test.cpp

    integration/parser_pipeline_test.cpp
    integration/itch_handler_cancel_test.cpp
    integration/itch_handler_delete_test.cpp
    integration/itch_handler_execute_test.cpp
    integration/itch_handler_replace_test.cpp
    integration/itch_replay_pipeline_test.cpp

   
)

target_include_directories(
    parser_tests
    PRIVATE
        ${PROJECT_ROOT}
)

target_link_libraries(
    parser_tests
    PRIVATE
        GTest::gtest_main
        ethernet_parser
        ipv4_parser
        udp_parser
        itch_parser
        replay
        gateway
        orderbook
)

include(GoogleTest)
gtest_discover_tests(parser_tests)