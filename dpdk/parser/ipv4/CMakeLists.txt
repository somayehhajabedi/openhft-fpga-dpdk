cmake_minimum_required(VERSION 3.20)

project(ipv4_parser_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    ipv4_test
    ipv4.cpp
    ipv4_test.cpp
)