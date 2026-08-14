
add_library(
    strategy
    STATIC
        strategy_engine.cpp
        simple_threshold_strategy.cpp
        market_data_strategy_consumer.cpp
)

target_include_directories(
    strategy
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    strategy
    PUBLIC
        market_data_pipeline
        orderbook
)
