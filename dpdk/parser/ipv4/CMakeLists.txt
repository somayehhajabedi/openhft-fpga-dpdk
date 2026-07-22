cmake_minimum_required(VERSION 3.20)

add_library(
    ipv4_parser
    STATIC
    ipv4.cpp
)

target_include_directories(
    ipv4_parser
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

add_executable(
    ipv4_test
    ipv4_test.cpp
)

target_link_libraries(
    ipv4_test
    PRIVATE
        ipv4_parser
)