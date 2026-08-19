#pragma once

#include "common/types.hpp"
#include "exchange/exchange_ouch_handler.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

class ExchangeTcpServer
{
public:
	
    ExchangeTcpServer(
        std::uint16_t port,
        AccountId accountId,
        ExchangeOuchHandler& handler);

    ~ExchangeTcpServer();

    bool start();

    bool pollOnce(
        int timeoutMilliseconds);

    void stop() noexcept;

    [[nodiscard]]
    std::uint16_t port() const noexcept;

private:
    static constexpr std::size_t EnterOrderSize = 47;
    static constexpr std::size_t MaxEvents = 32;

    struct ClientState
    {
        std::array<std::uint8_t, EnterOrderSize>
            receiveBuffer{};

        std::size_t receivedBytes{0};
    };

    bool setNonBlocking(
        int fd);

    bool acceptClients();

    bool handleClientReadable(
        int clientFd);

    bool sendAll(
        int clientFd,
        const std::uint8_t* data,
        std::size_t length);

    void closeClient(
        int clientFd);

    std::uint16_t port_;
    AccountId accountId_;
    ExchangeOuchHandler& handler_;

    int listenFd_{-1};
    int epollFd_{-1};

    std::unordered_map<
        int,
        ClientState> clients_;
};
