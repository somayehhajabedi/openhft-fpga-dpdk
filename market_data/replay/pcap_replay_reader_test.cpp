#include "pcap_replay_reader.hpp"
#include "../../dpdk/parser/ethernet/ethernet.hpp"
#include "../../dpdk/parser/ipv4/ipv4.hpp"
#include "../../dpdk/parser/udp/udp.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <exception>
#include <iomanip>
#include <algorithm>


int main()
{
    try
    {
        PcapReplayReader reader(
            "../../data/mac_dpdk_capture.pcapng");

        const std::uint8_t* data = nullptr;
        std::size_t length = 0;
        std::size_t packet_count = 0;

        while (reader.nextPacket(data, length))
        {
            ++packet_count;

            const EthernetHeader* ethernet =
                EthernetParser::parse(data, length);

            if (ethernet == nullptr)
            {
                std::cout
                    << "Packet "
                    << packet_count
                    << ": invalid Ethernet frame\n";

                continue;
            }

            if (EthernetParser::etherType(ethernet) != 0x0800)
            {
                continue;
            }

            const IPv4Header* ipv4 =
                IPv4Parser::parse(
                    EthernetParser::payload(ethernet),
                    length - sizeof(EthernetHeader));

            if (ipv4 == nullptr)
            {
                std::cout
                    << "Packet "
                    << packet_count
                    << ": invalid IPv4 packet\n";

                continue;
            }
            if (ipv4->protocol != 17)
            {
                continue;
            }

            const std::uint8_t* udp_data =
                IPv4Parser::payload(ipv4);

            const std::size_t udp_length =
                IPv4Parser::payloadLength(ipv4);

            const UDPHeader* udp =
                UDPParser::parse(
                    udp_data,
                    udp_length);

            if (udp == nullptr)
            {
                std::cout
                    << "Packet "
                    << packet_count
                    << ": invalid UDP packet\n";

                continue;
            }

            const std::uint8_t* payload =
            UDPParser::payload(udp);

            const std::size_t payloadLength =
                UDPParser::payloadLength(udp);

            if (payloadLength == 0)
            {
                continue;
            }

            switch (payload[0])
            {
                case 'A':
                    std::cout
                        << "Packet "
                        << packet_count
                        << ": Add Order message\n";
                    break;

                case 'X':
                    std::cout
                        << "Packet "
                        << packet_count
                        << ": Cancel message\n";
                    break;

                case 'D':
                    std::cout
                        << "Packet "
                        << packet_count
                        << ": Delete message\n";
                    break;

                case 'E':
                    std::cout
                        << "Packet "
                        << packet_count
                        << ": Executed message\n";
                    break;

                case 'U':
                    std::cout
                        << "Packet "
                        << packet_count
                        << ": Replace message\n";
                    break;

                default:
                    std::cout
                        << "Packet "
                        << packet_count
                        << ": Unknown ITCH type 0x"
                        << std::hex
                        << static_cast<int>(payload[0])
                        << std::dec
                        << '\n';
                    break;
            }
/*
            std::cout
                << "Packet "
                << packet_count
                << ", Payload (first 16 bytes): ";

            for (std::size_t i = 0;
                i < std::min<std::size_t>(16, payloadLength);
                ++i)
            {
                std::cout
                    << std::hex
                    << std::uppercase
                    << static_cast<int>(payload[i])
                    << ' ';
            }

            std::cout
                << std::dec
                << '\n';

            std::cout
                << "Packet "
                << packet_count
                << ", UDP source port = "
                << UDPParser::sourcePort(udp)
                << ", destination port = "
                << UDPParser::destinationPort(udp)
                << ", payload length = "
                << UDPParser::payloadLength(udp)
                << '\n';

            std::cout
                << "Packet "
                << packet_count
                << ", IPv4 version = "
                << static_cast<int>(
                    IPv4Parser::version(ipv4))
                << ", header length = "
                << static_cast<int>(
                    IPv4Parser::headerLengthBytes(ipv4))
                << ", total length = "
                << IPv4Parser::totalLength(ipv4)
                << '\n';

            std::cout
                << "Packet "
                << packet_count
                << ", length = "
                << length
                << ", EtherType = 0x"
                << std::hex
                << EthernetParser::etherType(ethernet)
                << std::dec
                << '\n';
                */
        }

        std::cout
            << "Total packets: "
            << packet_count
            << '\n';
    }
    catch (const std::exception& exception)
    {
        std::cerr
            << "Error: "
            << exception.what()
            << '\n';

        return 1;
    }

    return 0;
}
