
add_library(
    udp_parser
    STATIC
    udp.cpp
)

target_include_directories(
    udp_parser
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)