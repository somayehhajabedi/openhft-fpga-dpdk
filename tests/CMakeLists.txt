cmake_minimum_required(VERSION 3.20)

project(OpenHFTTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
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
    unit/risk/risk_manager_test.cpp
    unit/common/fixed_hash_map_test.cpp
    unit/orderbook/array_order_book_test.cpp
    unit/orderbook/order_pool_test.cpp
    unit/common/numa_utils_test.cpp
    unit/common/spsc_ring_buffer_test.cpp
    unit/common/spsc_queue_metrics_test.cpp
    unit/pipeline/market_data_book_consumer_test.cpp
    unit/strategy/simple_threshold_strategy_test.cpp
    unit/strategy/strategy_engine_test.cpp
    unit/strategy/market_data_strategy_consumer_test.cpp
    unit/execution/ouch_encoder_test.cpp
    unit/execution/ouch_execution_sink_test.cpp
    unit/execution/ouch_decoder_test.cpp
    unit/execution/ouch_response_dispatcher_test.cpp
    unit/logging/async_logger_test.cpp
    unit/parser/order_cancel_parser_test.cpp
    unit/mapper/order_cancel_mapper_test.cpp



    integration/parser_pipeline_test.cpp
    integration/itch_replay_pipeline_test.cpp
    integration/gateway_test.cpp
    integration/matching_engine_fifo_test.cpp
    integration/matching_engine_multilevel_test.cpp
    integration/matching_engine_resting_order_test.cpp
    integration/matching_engine_partial_incoming_test.cpp
    integration/matching_engine_partial_resting_test.cpp
    integration/market_data_pipeline_test.cpp
    integration/ouch_gateway_pipeline_test.cpp
    integration/tcp_ouch_transport_test.cpp
    integration/exchange_ouch_handler_test.cpp
    integration/exchange_tcp_server_test.cpp


)

target_include_directories(
    parser_tests
    PRIVATE
        ${PROJECT_ROOT}
)


target_compile_definitions(
    parser_tests
    PRIVATE
        TEST_PROJECT_ROOT="${PROJECT_ROOT}"
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
	matching_engine_core
        orderbook
        risk
	platform_utils
	market_data_pipeline
        strategy
	ouch
	exchange
)

include(GoogleTest)
gtest_discover_tests(parser_tests)
