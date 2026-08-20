add_library(
    logging
    STATIC
        async_logger.cpp
)

target_include_directories(
    logging
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    logging
    PUBLIC
        spdlog::spdlog
        Threads::Threads
)
