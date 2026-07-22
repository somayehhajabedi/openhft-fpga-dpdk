add_library(
    gateway
    STATIC
        gateway.cpp
        market_data/itch_handler.cpp
)

target_include_directories(
    gateway
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(
    gateway
    PUBLIC
        itch_parser
)