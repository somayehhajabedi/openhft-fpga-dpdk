#include "pcap_replay_reader.hpp"

#include <pcap/pcap.h>

#include <stdexcept>

#include <array>

PcapReplayReader::PcapReplayReader(
    const std::string& file_path)
{
    std::array<char, PCAP_ERRBUF_SIZE> error_buffer{};

    handle_ = pcap_open_offline(
        file_path.c_str(),
        error_buffer.data());

    if (handle_ == nullptr)
    {
        throw std::runtime_error(
            std::string{"Failed to open capture file: "}
            + error_buffer.data());
    }
}

PcapReplayReader::~PcapReplayReader()
{
    if (handle_ != nullptr)
    {
        pcap_close(handle_);
    }
}

bool PcapReplayReader::nextPacket(
    const std::uint8_t*& data,
    std::size_t& length)
{
    pcap_pkthdr* header = nullptr;
    const u_char* packet_data = nullptr;

    const int result = pcap_next_ex(
        handle_,
        &header,
        &packet_data);

    if (result == 1)
    {
        data = reinterpret_cast<const std::uint8_t*>(
            packet_data);

        length = static_cast<std::size_t>(
            header->caplen);

        return true;
    }

    if (result == PCAP_ERROR_BREAK)
    {
        data = nullptr;
        length = 0;
        return false;
    }

    if (result == PCAP_ERROR)
    {
        throw std::runtime_error(
            std::string{"Failed to read packet: "}
            + pcap_geterr(handle_));
    }

    return false;
}