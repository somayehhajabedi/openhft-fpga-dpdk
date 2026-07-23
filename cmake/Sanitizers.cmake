option(OPENHFT_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(OPENHFT_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(OPENHFT_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(OPENHFT_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if (OPENHFT_ENABLE_ASAN AND OPENHFT_ENABLE_TSAN)
    message(FATAL_ERROR "ASan and TSan cannot be enabled together")
endif()

if (OPENHFT_ENABLE_COVERAGE AND
    (OPENHFT_ENABLE_ASAN OR OPENHFT_ENABLE_UBSAN OR OPENHFT_ENABLE_TSAN))
    message(FATAL_ERROR "Coverage must use a separate build from sanitizers")
endif()

set(OPENHFT_SANITIZER_FLAGS "")

if (OPENHFT_ENABLE_ASAN)
    list(APPEND OPENHFT_SANITIZER_FLAGS
        -fsanitize=address
        -fno-omit-frame-pointer
    )
endif()

if (OPENHFT_ENABLE_UBSAN)
    list(APPEND OPENHFT_SANITIZER_FLAGS
        -fsanitize=undefined
        -fno-omit-frame-pointer
    )
endif()

if (OPENHFT_ENABLE_TSAN)
    list(APPEND OPENHFT_SANITIZER_FLAGS
        -fsanitize=thread
        -fno-omit-frame-pointer
    )
endif()

if (OPENHFT_SANITIZER_FLAGS)
    add_compile_options(${OPENHFT_SANITIZER_FLAGS})
    add_link_options(${OPENHFT_SANITIZER_FLAGS})
endif()

if (OPENHFT_ENABLE_COVERAGE)
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Coverage is supported only with GCC or Clang")
    endif()

    add_compile_options(
        --coverage
        -O0
        -g
    )

    add_link_options(--coverage)
endif()
