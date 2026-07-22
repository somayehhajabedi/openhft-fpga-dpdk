#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "market_data/replay/itch_replay_reader.hpp"

namespace
{

void writeMessage(
    std::ofstream& stream,
    const std::vector<std::uint8_t>& payload)
{
    const auto networkLength =
        htons(static_cast<std::uint16_t>(payload.size()));

    stream.write(
        reinterpret_cast<const char*>(&networkLength),
        sizeof(networkLength));

    stream.write(
        reinterpret_cast<const char*>(payload.data()),
        static_cast<std::streamsize>(payload.size()));
}

} // namespace

TEST(ItchReplayReaderTest, OpensValidFile)
{
    const std::string path = "itch_replay_open_test.bin";

    {
        std::ofstream stream(path, std::ios::binary);
        ASSERT_TRUE(stream.is_open());
    }

    ItchReplayReader reader(path);

    EXPECT_TRUE(reader.isOpen());

    std::remove(path.c_str());
}

TEST(ItchReplayReaderTest, FailsToOpenMissingFile)
{
    const std::string path = "missing_itch_replay_file.bin";

    std::remove(path.c_str());

    ItchReplayReader reader(path);

    EXPECT_FALSE(reader.isOpen());
}

TEST(ItchReplayReaderTest, ReadsTwoMessages)
{
    const std::string path = "itch_replay_two_messages.bin";

    const std::vector<std::uint8_t> first{
        'A', 0x01, 0x02
    };

    const std::vector<std::uint8_t> second{
        'D', 0x03
    };

    {
        std::ofstream stream(path, std::ios::binary);
        ASSERT_TRUE(stream.is_open());

        writeMessage(stream, first);
        writeMessage(stream, second);
    }

    ItchReplayReader reader(path);

    ASSERT_TRUE(reader.isOpen());

    std::vector<std::uint8_t> message;

    ASSERT_TRUE(reader.readNext(message));
    EXPECT_EQ(message, first);

    ASSERT_TRUE(reader.readNext(message));
    EXPECT_EQ(message, second);

    EXPECT_FALSE(reader.readNext(message));

    std::remove(path.c_str());
}

TEST(ItchReplayReaderTest, RejectsTruncatedLengthPrefix)
{
    const std::string path =
        "itch_replay_truncated_length.bin";

    {
        std::ofstream stream(path, std::ios::binary);
        ASSERT_TRUE(stream.is_open());

        const std::uint8_t oneByte = 0x00;

        stream.write(
            reinterpret_cast<const char*>(&oneByte),
            sizeof(oneByte));
    }

    ItchReplayReader reader(path);

    std::vector<std::uint8_t> message{
        0xFF
    };

    EXPECT_FALSE(reader.readNext(message));

    std::remove(path.c_str());
}

TEST(ItchReplayReaderTest, RejectsTruncatedPayload)
{
    const std::string path =
        "itch_replay_truncated_payload.bin";

    {
        std::ofstream stream(path, std::ios::binary);
        ASSERT_TRUE(stream.is_open());

        const std::uint16_t networkLength = htons(5);

        stream.write(
            reinterpret_cast<const char*>(&networkLength),
            sizeof(networkLength));

        const std::uint8_t incompletePayload[2]{
            'A', 0x01
        };

        stream.write(
            reinterpret_cast<const char*>(incompletePayload),
            sizeof(incompletePayload));
    }

    ItchReplayReader reader(path);

    std::vector<std::uint8_t> message{
        0xFF
    };

    EXPECT_FALSE(reader.readNext(message));
    EXPECT_TRUE(message.empty());

    std::remove(path.c_str());
}

TEST(ItchReplayReaderTest, RejectsZeroLengthMessage)
{
    const std::string path =
        "itch_replay_zero_length.bin";

    {
        std::ofstream stream(path, std::ios::binary);
        ASSERT_TRUE(stream.is_open());

        const std::uint16_t networkLength = htons(0);

        stream.write(
            reinterpret_cast<const char*>(&networkLength),
            sizeof(networkLength));
    }

    ItchReplayReader reader(path);

    std::vector<std::uint8_t> message;

    EXPECT_FALSE(reader.readNext(message));

    std::remove(path.c_str());
}