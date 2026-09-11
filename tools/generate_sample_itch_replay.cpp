/*
 * Sample ITCH Replay Generator
 * ----------------------------
 * Generates a small two-sided ITCH replay file for exercising
 * the trading runtime end-to-end.
 *
 * The replay contains one bid and one ask so the local order book
 * becomes two-sided and the market-feature extractor can calculate
 * spread, midpoint, imbalance, and microprice.
 *
 * Quantities are intentionally asymmetric to create a strong
 * microstructure signal and increase the chance that the strategy
 * generates an OrderIntent.
 */

#include "dpdk/parser/itch/messages/add_order.hpp"

#include <arpa/inet.h>
#include <endian.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>


template <typename Message>
void writeMessage(
    std::ofstream& stream,
    const Message& message)
{
    const auto messageLength =
        static_cast<std::uint16_t>(
            sizeof(Message));

    const std::uint16_t networkLength =
        htons(messageLength);

    stream.write(
        reinterpret_cast<const char*>(
            &networkLength),
        sizeof(networkLength));

    stream.write(
        reinterpret_cast<const char*>(
            &message),
        sizeof(message));
}


AddOrderWireMessage makeAddOrder(
    std::uint64_t orderReferenceNumber,
    char side,
    std::uint32_t shares,
    std::uint32_t price)
{
    AddOrderWireMessage message{};

    message.message_type = 'A';

    message.order_reference_number =
        htobe64(orderReferenceNumber);

    message.buy_sell_indicator =
        side;

    message.shares =
        htonl(shares);

    const char symbol[8] = {
        'A', 'A', 'P', 'L',
        ' ', ' ', ' ', ' '
    };

    std::copy_n(
        symbol,
        8,
        message.stock);

    message.price =
        htonl(price);

    return message;
}


int main()
{
    const std::string path =
        "sample_itch_replay.bin";


    const AddOrderWireMessage bid =
        makeAddOrder(
            5001,
            'B',
            900,
            98);


    const AddOrderWireMessage ask =
        makeAddOrder(
            5002,
            'S',
            100,
            99);


    std::ofstream stream(
        path,
        std::ios::binary);


    if (!stream.is_open())
    {
        std::cerr
            << "Failed to create "
            << path
            << '\n';

        return 1;
    }


    writeMessage(
        stream,
        bid);


    writeMessage(
        stream,
        ask);


    std::cout
        << "Created "
        << path
        << " with two-sided market data\n";

    return 0;
}