#include "execution/ouch/tls_ouch_transport.hpp"

#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>

#include <sys/socket.h>
#include <unistd.h>

#include <utility>

namespace ouch
{

TlsOuchTransport::TlsOuchTransport(
    std::string host,
    std::uint16_t port,
    std::string caCertificatePath)
    :
    host_(std::move(host)),
    port_(port),
    caCertificatePath_(
        std::move(caCertificatePath))
{
}

TlsOuchTransport::~TlsOuchTransport()
{
    close();
}

bool TlsOuchTransport::connect()
{
    if (isConnected())
    {
        return true;
    }

    //
    // Create a TLS client context.
    //
    sslContext_ =
        SSL_CTX_new(
            TLS_client_method());

    if (sslContext_ == nullptr)
    {
        return false;
    }

    //
    // Require verification of the server certificate.
    //
    SSL_CTX_set_verify(
        sslContext_,
        SSL_VERIFY_PEER,
        nullptr);

    //
    // Load the CA certificate that we trust.
    //
    if (SSL_CTX_load_verify_locations(
            sslContext_,
            caCertificatePath_.c_str(),
            nullptr) != 1)
    {
        close();
        return false;
    }

    //
    // Create the underlying TCP socket.
    //
    socketFd_ =
        ::socket(
            AF_INET,
            SOCK_STREAM,
            0);

    if (socketFd_ < 0)
    {
        close();
        return false;
    }

    sockaddr_in address{};

    address.sin_family =
        AF_INET;

    address.sin_port =
        htons(port_);

    if (::inet_pton(
            AF_INET,
            host_.c_str(),
            &address.sin_addr) != 1)
    {
        close();
        return false;
    }

    //
    // Establish the TCP connection first.
    //
    if (::connect(
            socketFd_,
            reinterpret_cast<sockaddr*>(
                &address),
            sizeof(address)) < 0)
    {
        close();
        return false;
    }

    //
    // Create a TLS session for this TCP connection.
    //
    ssl_ =
        SSL_new(
            sslContext_);

    if (ssl_ == nullptr)
    {
        close();
        return false;
    }

    //
    // Bind the TLS session to the TCP socket.
    //
    if (SSL_set_fd(
            ssl_,
            socketFd_) != 1)
    {
        close();
        return false;
    }

    //
    // Verify that the certificate belongs to the
    // IP address we are connecting to.
    //
    X509_VERIFY_PARAM* verifyParam =
        SSL_get0_param(
            ssl_);

    if (verifyParam == nullptr)
    {
        close();
        return false;
    }

    if (X509_VERIFY_PARAM_set1_ip_asc(
            verifyParam,
            host_.c_str()) != 1)
    {
        close();
        return false;
    }

    //
    // Perform the TLS handshake.
    //
    if (SSL_connect(
            ssl_) != 1)
    {
        close();
        return false;
    }

    //
    // Confirm that certificate-chain and identity
    // verification succeeded.
    //
    if (SSL_get_verify_result(
            ssl_) != X509_V_OK)
    {
        close();
        return false;
    }

    return true;
}

bool TlsOuchTransport::send(
    const std::uint8_t* data,
    std::size_t length)
{
    if (!isConnected() ||
        data == nullptr)
    {
        return false;
    }

    std::size_t totalSent = 0;

    while (totalSent < length)
    {
        const int sent =
            SSL_write(
                ssl_,
                data + totalSent,
                static_cast<int>(
                    length - totalSent));

        if (sent <= 0)
        {
            return false;
        }

        totalSent +=
            static_cast<std::size_t>(
                sent);
    }

    return true;
}

bool TlsOuchTransport::receive(
    std::uint8_t* data,
    std::size_t length)
{
    if (!isConnected() ||
        data == nullptr)
    {
        return false;
    }

    std::size_t totalReceived = 0;

    while (totalReceived < length)
    {
        const int received =
            SSL_read(
                ssl_,
                data + totalReceived,
                static_cast<int>(
                    length - totalReceived));

        if (received <= 0)
        {
            return false;
        }

        totalReceived +=
            static_cast<std::size_t>(
                received);
    }

    return true;
}

void TlsOuchTransport::close() noexcept
{
    if (ssl_ != nullptr)
    {
        SSL_shutdown(
            ssl_);

        SSL_free(
            ssl_);

        ssl_ = nullptr;
    }

    if (socketFd_ >= 0)
    {
        ::close(
            socketFd_);

        socketFd_ = -1;
    }

    if (sslContext_ != nullptr)
    {
        SSL_CTX_free(
            sslContext_);

        sslContext_ = nullptr;
    }
}

bool TlsOuchTransport::isConnected() const noexcept
{
    return
        socketFd_ >= 0 &&
        ssl_ != nullptr;
}

} // namespace ouch