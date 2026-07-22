add_library(
    itch_parser
    STATIC
        itch.cpp

        messages/add_order.cpp
        messages/order_cancel_parser.cpp
        messages/order_delete_parser.cpp
        messages/order_executed_parser.cpp
        messages/order_replace_parser.cpp

        mapper/add_order_mapper.cpp
        mapper/order_cancel_mapper.cpp
        mapper/order_delete_mapper.cpp
        mapper/order_executed_mapper.cpp
        mapper/order_replace_mapper.cpp
)

target_include_directories(
    itch_parser
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)