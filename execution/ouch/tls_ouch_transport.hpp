#pragma once

#include "execution/ouch/ouch_transport.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

struct ssl_ctx_st;
struct ssl_st;

namespace ouch
{

class TlsOuchTransport final : public OuchTransport
{
public:
    TlsOuchTransport(
        std::string host,
        std::uint16_t port,
        std::string caCertificatePath);

    ~TlsOuchTransport() override;

    TlsOuchTransport(
        const TlsOuchTransport&) = delete;

    TlsOuchTransport& operator=(
        const TlsOuchTransport&) = delete;

    TlsOuchTransport(
        TlsOuchTransport&&) = delete;

    TlsOuchTransport& operator=(
        TlsOuchTransport&&) = delete;

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
    std::string caCertificatePath_;

    int socketFd_{-1};

    ssl_ctx_st* sslContext_{nullptr};
    ssl_st* ssl_{nullptr};
};

} // namespace ouch