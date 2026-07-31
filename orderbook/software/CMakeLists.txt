# ------------------------
# OrderBook library
# ------------------------

add_library(
    orderbook
    STATIC
        array_order_book.cpp
	order_pool.cpp
)

target_include_directories(
    orderbook
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}
)

# ------------------------
# Matching Engine library
# ------------------------

add_library(
    matching_engine_core
    STATIC
        matching_engine.cpp
        ../../market_data/trade_publisher.cpp
        ../../sequencer/sequencer.cpp
        ../../dispatcher/event_dispatcher.cpp
        ../../journal/journal.cpp
        ../../position/position_manager.cpp
)

target_include_directories(
    matching_engine_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    matching_engine_core
    PUBLIC
        orderbook
)
