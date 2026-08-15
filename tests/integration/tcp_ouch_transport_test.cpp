#include <gtest/gtest.h>

#include "execution/ouch/tcp_ouch_transport.hpp"

#include <arpa/inet.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

TEST(
    TcpOuchTransportTest,
    SendsBytesToLoopbackServer)
{
    const int serverFd =
        ::socket(
            AF_INET,
            SOCK_STREAM,
            0);

    ASSERT_GE(serverFd, 0);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr =
        htonl(INADDR_LOOPBACK);
    serverAddress.sin_port = 0;

    ASSERT_EQ(
        ::bind(
            serverFd,
            reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress)),
        0);

    ASSERT_EQ(
        ::listen(
            serverFd,
            1),
        0);

    socklen_t addressLength =
        sizeof(serverAddress);

    ASSERT_EQ(
        ::getsockname(
            serverFd,
            reinterpret_cast<sockaddr*>(&serverAddress),
            &addressLength),
        0);

    const std::uint16_t port =
        ntohs(serverAddress.sin_port);

    std::array<std::uint8_t, 47> received{};
    std::size_t receivedLength = 0;

    std::thread serverThread(
        [&]()
        {
            const int clientFd =
                ::accept(
                    serverFd,
                    nullptr,
                    nullptr);

            ASSERT_GE(clientFd, 0);

            while (receivedLength < received.size())
            {
                const ssize_t count =
                    ::recv(
                        clientFd,
                        received.data() + receivedLength,
                        received.size() - receivedLength,
                        0);

                if (count <= 0)
                {
                    break;
                }

                receivedLength +=
                    static_cast<std::size_t>(count);
            }

            ::close(clientFd);
        });

    ouch::TcpOuchTransport transport(
        "127.0.0.1",
        port);

    ASSERT_TRUE(
        transport.connect());

    std::array<std::uint8_t, 47> message{};

    for (std::size_t i = 0;
         i < message.size();
         ++i)
    {
        message[i] =
            static_cast<std::uint8_t>(i);
    }

    EXPECT_TRUE(
        transport.send(
            message.data(),
            message.size()));

    transport.close();

    serverThread.join();

    EXPECT_EQ(
        receivedLength,
        message.size());

    EXPECT_EQ(
        received,
        message);

    ::close(serverFd);
}
