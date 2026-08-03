
add_library(
    platform_utils
    STATIC
        thread_affinity.cpp
        numa_utils.cpp
)

target_include_directories(
    platform_utils
    PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    platform_utils
    PUBLIC
        Threads::Threads
        numa
)