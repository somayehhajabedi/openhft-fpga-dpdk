add_library(risk
    risk_manager.cpp
)

target_include_directories(risk
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(risk
    PUBLIC
        orderbook
)