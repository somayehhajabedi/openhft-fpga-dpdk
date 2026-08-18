add_library(
    gateway
    STATIC
        gateway.cpp
        gateway_order_intent_sink.cpp
        matching_engine_execution_sink.cpp
)

target_include_directories(
    gateway
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    gateway
    PUBLIC
        matching_engine_core
        risk
        itch_parser
        strategy
)
