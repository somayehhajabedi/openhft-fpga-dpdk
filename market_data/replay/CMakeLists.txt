cmake_minimum_required(VERSION 3.20)

project(Replay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PCAP REQUIRED libpcap)

# ------------------------
# Replay library
# ------------------------

add_library(
    replay
    STATIC
        itch_replay_reader.cpp
        itch_replay_dispatcher.cpp
)

target_include_directories(
    replay
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

# ------------------------
# PCAP replay test
# ------------------------

add_executable(
    pcap_replay_reader_test
    pcap_replay_reader.cpp
    pcap_replay_reader_test.cpp
)

target_include_directories(
    pcap_replay_reader_test
    PRIVATE
        ${PCAP_INCLUDE_DIRS}
)

target_link_libraries(
    pcap_replay_reader_test
    PRIVATE
        replay
        ethernet_parser
        ipv4_parser
        udp_parser
        ${PCAP_LIBRARIES}
)