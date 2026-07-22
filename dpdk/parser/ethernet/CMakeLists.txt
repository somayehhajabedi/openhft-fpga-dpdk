cmake_minimum_required(VERSION 3.20)


add_library(
    ethernet_parser
    STATIC
    ethernet.cpp
)

target_include_directories(
    ethernet_parser
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

add_executable(
    ethernet_test
    ethernet_test.cpp
)

target_link_libraries(
    ethernet_test
    PRIVATE
        ethernet_parser
)