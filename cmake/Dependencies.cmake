include(FetchContent)

set(
    BENCHMARK_ENABLE_TESTING
    OFF
    CACHE BOOL
    ""
    FORCE
)

set(
    BENCHMARK_ENABLE_GTEST_TESTS
    OFF
    CACHE BOOL
    ""
    FORCE
)

set(
    BENCHMARK_ENABLE_INSTALL
    OFF
    CACHE BOOL
    ""
    FORCE
)



FetchContent_Declare(
    google_benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(
    google_benchmark
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(
    spdlog
)



