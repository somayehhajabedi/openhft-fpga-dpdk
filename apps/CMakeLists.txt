add_executable(
    matching_engine_demo
    matching_engine_demo.cpp
)

target_include_directories(
    matching_engine_demo
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    matching_engine_demo
    PRIVATE
        gateway
        matching_engine_core
        risk
        itch_parser
)