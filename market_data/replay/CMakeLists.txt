cmake_minimum_required(VERSION 3.20)

project(pcap_replay_reader_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PCAP REQUIRED libpcap)

add_executable(
    pcap_replay_reader_test
    pcap_replay_reader.cpp
    pcap_replay_reader_test.cpp
    ../../dpdk/parser/ethernet/ethernet.cpp
    ../../dpdk/parser/ipv4/ipv4.cpp
    ../../dpdk/parser/udp/udp.cpp
    
)

target_include_directories(
    pcap_replay_reader_test
    PRIVATE
    ${PCAP_INCLUDE_DIRS}
)

target_link_libraries(
    pcap_replay_reader_test
    PRIVATE
    ${PCAP_LIBRARIES}
)