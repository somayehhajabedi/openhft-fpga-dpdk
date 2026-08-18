add_executable(
    matching_engine_demo
    matching_engine_demo.cpp
)

target_include_directories(
    matching_engine_demo
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    matching_engine_demo
    PRIVATE
        gateway
        matching_engine_core
        risk
        itch_parser
)
add_executable(
    trading_runtime
    trading_runtime.cpp
)

target_include_directories(
    trading_runtime
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    trading_runtime
    PRIVATE
        market_data_pipeline
        orderbook
	strategy
	replay
	itch_parser
)
