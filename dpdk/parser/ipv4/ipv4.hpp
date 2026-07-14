#pragma once

#include <cstddef>
#include <cstdint>

#pragma pack(push, 1)

struct IPv4Header
{
    std::uint8_t version_ihl;
    std::uint8_t dscp_ecn;
    std::uint16_t total_length;
    std::uint16_t identification;
    std::uint16_t flags_fragment_offset;
    std::uint8_t ttl;
    std::uint8_t protocol;
    std::uint16_t header_checksum;
    std::uint32_t source_address;
    std::uint32_t destination_address;
};

#pragma pack(pop)

static_assert(sizeof(IPv4Header) == 20);

class IPv4Parser
{
public:
    static const IPv4Header* parse(const std::uint8_t* data,
                                   std::size_t length);

    static std::uint8_t version(const IPv4Header* header);

    static std::uint8_t headerLengthBytes(const IPv4Header* header);

    static std::uint16_t totalLength(const IPv4Header* header);

    static const std::uint8_t*
    payload(const IPv4Header* header);

    static std::size_t
    payloadLength(const IPv4Header* header);

};