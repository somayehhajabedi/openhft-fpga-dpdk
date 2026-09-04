find_package(OpenSSL REQUIRED)

add_library(
    ouch
    STATIC
        ouch_encoder.cpp
        ouch_decoder.cpp
        ouch_execution_sink.cpp
        tcp_ouch_transport.cpp
        tls_ouch_transport.cpp
        decoders/enter_order_decoder.cpp
        decoders/accepted_decoder.cpp
        decoders/rejected_decoder.cpp
        decoders/executed_decoder.cpp
        decoders/canceled_decoder.cpp
        ouch_response_dispatcher.cpp
        accepted_encoder.cpp
        executed_encoder.cpp
)

target_include_directories(
    ouch
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    ouch
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
)