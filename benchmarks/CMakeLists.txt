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
	    platform_utils
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


add_executable(
    bitmap_benchmark
    bitmap_benchmark.cpp
)

target_link_libraries(
    bitmap_benchmark
    PRIVATE
        orderbook
        benchmark::benchmark
        benchmark::benchmark_main
)
add_executable(
    order_pool_benchmark
    order_pool_benchmark.cpp
)

target_link_libraries(
    order_pool_benchmark
    PRIVATE
        orderbook
        benchmark::benchmark
        benchmark::benchmark_main
)


add_executable(
    huge_page_benchmark
    huge_page_benchmark.cpp
)

target_link_libraries(
    huge_page_benchmark
    PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
)

add_executable(
    false_sharing_benchmark
    false_sharing_benchmark.cpp
)

target_link_libraries(
    false_sharing_benchmark
    PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
        Threads::Threads
)

add_executable(
    allocator_benchmark
    allocator_benchmark.cpp
)

target_link_libraries(
    allocator_benchmark
    PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
)

add_executable(
    numa_benchmark
    numa_benchmark.cpp
)

target_link_libraries(
    numa_benchmark
    PRIVATE
        platform_utils
        benchmark::benchmark
        benchmark::benchmark_main
)

add_executable(
    lockfree_queue_benchmark
    lockfree_queue_benchmark.cpp
)

target_include_directories(
    lockfree_queue_benchmark
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    lockfree_queue_benchmark
    PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
        Threads::Threads
)





