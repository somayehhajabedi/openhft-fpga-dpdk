add_library(
    market_data_pipeline
    STATIC
        dispatcher.cpp
        market_data_pipeline.cpp
        
)

target_include_directories(
    market_data_pipeline
    
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)