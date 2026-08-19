#pragma once

#include "execution/ouch/ouch_transport.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace ouch
{

class TcpOuchTransport final : public OuchTransport
{
public:
    TcpOuchTransport(
        std::string host,
        std::uint16_t port);

    ~TcpOuchTransport() override;

    TcpOuchTransport(
        const TcpOuchTransport&) = delete;

    TcpOuchTransport& operator=(
        const TcpOuchTransport&) = delete;

    TcpOuchTransport(
        TcpOuchTransport&&) = delete;

    TcpOuchTransport& operator=(
        TcpOuchTransport&&) = delete;

    bool connect();

    bool send(
        const std::uint8_t* data,
        std::size_t length) override;

    bool receive(
        std::uint8_t* data,
        std::size_t length);

    void close() noexcept;

    [[nodiscard]]
    bool isConnected() const noexcept;

private:
    std::string host_;
    std::uint16_t port_{};

    int socketFd_{-1};
};

} // namespace ouch
