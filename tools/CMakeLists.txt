# Sample ITCH replay generator tool.
#
# This utility generates a small binary ITCH replay file used to
# exercise the market-data pipeline and strategy path locally.

add_executable(
    generate_sample_itch_replay
    generate_sample_itch_replay.cpp
)

target_include_directories(
    generate_sample_itch_replay
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)
