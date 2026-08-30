cmake_minimum_required(VERSION 3.20)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

add_library(
    dpdk_receiver_core
    STATIC
        receiver.cpp
        ../parser/ethernet/ethernet.cpp
        ../parser/ipv4/ipv4.cpp
        ../parser/udp/udp.cpp
)

target_compile_options(
    dpdk_receiver_core
    PRIVATE
        -march=native
)

target_include_directories(
    dpdk_receiver_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${DPDK_INCLUDE_DIRS}
)

target_link_directories(
    dpdk_receiver_core
    PUBLIC
        ${DPDK_LIBRARY_DIRS}
)

target_link_libraries(
    dpdk_receiver_core
    PUBLIC
        ${DPDK_LIBRARIES}
        platform_utils
)

add_executable(
    receiver
    main.cpp
)

target_link_libraries(
    receiver
    PRIVATE
        dpdk_receiver_core
)
