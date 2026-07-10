cmake_minimum_required(VERSION 3.20)

project(MatchingEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    matching_engine

    matching_engine.cpp
    array_order_book.cpp
)
