cmake_minimum_required(VERSION 3.20)

project(dpdk_receiver)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

add_executable(receiver
    main.cpp
    receiver.cpp

    ../parser/ethernet/ethernet.cpp
    ../parser/ipv4/ipv4.cpp
    ../parser/udp/udp.cpp
)

target_compile_options(receiver PRIVATE -march=native)

target_include_directories(receiver PRIVATE
    ${DPDK_INCLUDE_DIRS}
)

target_link_directories(receiver PRIVATE
    ${DPDK_LIBRARY_DIRS}
)

target_link_libraries(receiver PRIVATE
    ${DPDK_LIBRARIES}
)
