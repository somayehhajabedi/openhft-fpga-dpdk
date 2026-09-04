#pragma once

#include "common/types.hpp"
#include "exchange/exchange_ouch_handler.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct ssl_ctx_st;
struct ssl_st;

class ExchangeTcpServer
{
public:
    ExchangeTcpServer(
        std::uint16_t port,
        AccountId accountId,
        ExchangeOuchHandler& handler,
        std::string certificatePath = {},
        std::string privateKeyPath = {});

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
        ssl_st* ssl{nullptr};

        bool tlsHandshakeComplete{false};

        std::array<std::uint8_t, EnterOrderSize>
            receiveBuffer{};

        std::size_t receivedBytes{0};

        std::vector<std::uint8_t>
            sendBuffer{};

        std::size_t sentBytes{0};
    };

    bool initializeTls();

    bool setNonBlocking(
        int fd);

    bool acceptClients();

    bool progressTlsHandshake(
        int clientFd);

    bool handleClientReadable(
        int clientFd);

    bool sendAll(
        int clientFd,
        const std::uint8_t* data,
        std::size_t length);

    bool updateClientEvents(
        int clientFd,
        std::uint32_t events);

    void closeClient(
        int clientFd);

    bool flushPendingWrite(
    int clientFd);

    std::uint16_t port_;
    AccountId accountId_;
    ExchangeOuchHandler& handler_;

    std::string certificatePath_;
    std::string privateKeyPath_;

    ssl_ctx_st* sslContext_{nullptr};

    int listenFd_{-1};
    int epollFd_{-1};

    std::unordered_map<
        int,
        ClientState> clients_;
};