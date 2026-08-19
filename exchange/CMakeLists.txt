add_library(
    exchange
    STATIC
        exchange_ouch_handler.cpp
        exchange_execution_listener.cpp
        exchange_tcp_server.cpp
)

target_include_directories(
    exchange
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    exchange
    PUBLIC
        ouch
        matching_engine_core
)
