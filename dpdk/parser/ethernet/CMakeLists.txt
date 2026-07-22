cmake_minimum_required(VERSION 3.20)

project(ethernet_parser_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    ethernet_test
    ethernet.cpp
    ethernet_test.cpp
)