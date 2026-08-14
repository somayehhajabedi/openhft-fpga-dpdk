find_package(Threads REQUIRED)

add_library(
    market_data_pipeline
    STATIC
        dispatcher.cpp
        market_data_pipeline.cpp
	market_data_book_consumer.cpp

)

target_include_directories(
    market_data_pipeline
    PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/orderbook/software
)

target_link_libraries(
    market_data_pipeline
    PUBLIC
        Threads::Threads
        matching_engine_core
        platform_utils
)
