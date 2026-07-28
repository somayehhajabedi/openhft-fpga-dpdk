find_package(benchmark REQUIRED)

add_executable(
    orderbook_benchmark
    orderbook_benchmark.cpp
)

target_link_libraries(
    orderbook_benchmark
    PRIVATE
        orderbook
        benchmark::benchmark
        benchmark::benchmark_main
)

add_executable(
    matching_engine_benchmark
    matching_engine_benchmark.cpp
)

target_link_libraries(
    matching_engine_benchmark
    PRIVATE
        matching_engine_core
        benchmark::benchmark
)

add_executable(
    fixed_hash_map_benchmark
    fixed_hash_map_benchmark.cpp
)

target_include_directories(
    fixed_hash_map_benchmark
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(
    fixed_hash_map_benchmark
    PRIVATE
        benchmark::benchmark
)

add_executable(
    matching_engine_profile
    matching_engine_profile.cpp
)

target_link_libraries(
    matching_engine_profile
    PRIVATE
        matching_engine_core
)

add_executable(
    pipeline_benchmark
    pipeline_benchmark.cpp
)

target_link_libraries(
    pipeline_benchmark
    PRIVATE
        replay
        gateway
        benchmark::benchmark
        benchmark::benchmark_main
)


