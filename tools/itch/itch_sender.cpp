#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>

static std::uint64_t hostToBigEndian64(std::uint64_t value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap64(value);
#else
    return value;
#endif
}

#pragma pack(push, 1)
struct AddOrderWireMessage
{
    char message_type;
    std::uint16_t stock_locate;
    std::uint16_t tracking_number;
    std::uint8_t timestamp[6];
    std::uint64_t order_reference_number;
    char buy_sell_indicator;
    std::uint32_t shares;
    char stock[8];
    std::uint32_t price;
};
#pragma pack(pop)

static_assert(sizeof(AddOrderWireMessage) == 36);

int main()
{
    AddOrderWireMessage message{};

    message.message_type = 'A';
    message.stock_locate = htons(1);
    message.tracking_number = htons(1);

    std::memset(message.timestamp, 0, sizeof(message.timestamp));

    message.order_reference_number =
        hostToBigEndian64(12345);

    message.buy_sell_indicator = 'B';
    message.shares = htonl(100);

    std::memcpy(message.stock, "AAPL    ", 8);

    message.price = htonl(50000);

    const int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in destination{};
    destination.sin_family = AF_INET;
    destination.sin_port = htons(9000);

    if (inet_pton(
            AF_INET,
            "192.168.50.2",
            &destination.sin_addr) != 1)
    {
        std::cerr << "Invalid destination IP\n";
        close(sock);
        return 1;
    }

    const ssize_t sent =
        sendto(
            sock,
            &message,
            sizeof(message),
            0,
            reinterpret_cast<sockaddr*>(&destination),
            sizeof(destination));

    if (sent != sizeof(message))
    {
        perror("sendto");
        close(sock);
        return 1;
    }

    std::cout
        << "Sent ITCH AddOrder: "
        << "id=12345 "
        << "symbol=AAPL "
        << "side=BUY "
        << "shares=100 "
        << "price=1000000 "
        << "bytes=" << sent
        << '\n';

    close(sock);
}
