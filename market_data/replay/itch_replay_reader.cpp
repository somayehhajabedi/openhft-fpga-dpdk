#include "itch_replay_reader.hpp"

#include <arpa/inet.h>

ItchReplayReader::ItchReplayReader(
    const std::string& filePath)
    : stream_(filePath, std::ios::binary)
{
}

bool ItchReplayReader::isOpen() const
{
    return stream_.is_open();
}

bool ItchReplayReader::readNext(
    std::vector<std::uint8_t>& message)
{
    std::uint16_t networkLength{};

    stream_.read(
        reinterpret_cast<char*>(&networkLength),
        sizeof(networkLength));

    if (stream_.eof())
        return false;

    if (!stream_)
        return false;

    const std::uint16_t messageLength =
        ntohs(networkLength);

    if (messageLength == 0)
        return false;

    message.resize(messageLength);

    stream_.read(
        reinterpret_cast<char*>(message.data()),
        messageLength);

    if (!stream_)
    {
        message.clear();
        return false;
    }

    return true;
}

