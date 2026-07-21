#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

struct pcap;
using pcap_t = struct pcap;

class PcapReplayReader
{
public:
    explicit PcapReplayReader(const std::string& file_path);

    ~PcapReplayReader();

    PcapReplayReader(const PcapReplayReader&) = delete;
    PcapReplayReader& operator=(const PcapReplayReader&) = delete;

    bool nextPacket(
        const std::uint8_t*& data,
        std::size_t& length);

private:
    pcap_t* handle_{nullptr};
};