cmake_minimum_required(VERSION 3.20)

project(MatchingEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------
# OrderBook library
# ------------------------

add_library(
    orderbook
    STATIC
        array_order_book.cpp
)

target_include_directories(
    orderbook
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
)

# ------------------------
# Matching Engine executable
# ------------------------

add_executable(
    matching_engine
        matching_engine.cpp
        ../../gateway/gateway.cpp
        ../../risk/risk_manager.cpp
        ../../market_data/trade_publisher.cpp
        ../../sequencer/sequencer.cpp
        ../../dispatcher/event_dispatcher.cpp
        ../../journal/journal.cpp
        ../../position/position_manager.cpp
)

target_link_libraries(
    matching_engine
    PRIVATE
        orderbook
)
