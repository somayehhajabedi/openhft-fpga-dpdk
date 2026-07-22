find_package(benchmark REQUIRED)

add_executable(orderbook_benchmark
    orderbook_benchmark.cpp
)

target_link_libraries(orderbook_benchmark
    PRIVATE
        orderbook
        benchmark::benchmark
        benchmark::benchmark_main
)
