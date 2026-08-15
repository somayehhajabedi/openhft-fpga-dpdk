#include "execution/ouch/tcp_ouch_transport.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace ouch
{

TcpOuchTransport::TcpOuchTransport(
    std::string host,
    std::uint16_t port)
    :
    host_(std::move(host)),
    port_(port)
{
}

TcpOuchTransport::~TcpOuchTransport()
{
    close();
}

bool TcpOuchTransport::connect()
{
    if (isConnected())
    {
        return true;
    }

    socketFd_ =
        ::socket(
            AF_INET,
            SOCK_STREAM,
            0);

    if (socketFd_ < 0)
    {
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port_);

    const int result =
        ::inet_pton(
            AF_INET,
            host_.c_str(),
            &address.sin_addr);

    if (result != 1)
    {
        close();
        return false;
    }

    if (::connect(
            socketFd_,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) < 0)
    {
        close();
        return false;
    }

    return true;
}

bool TcpOuchTransport::send(
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
        const ssize_t sent =
            ::send(
                socketFd_,
                data + totalSent,
                length - totalSent,
                MSG_NOSIGNAL);

        if (sent > 0)
        {
            totalSent +=
                static_cast<std::size_t>(sent);

            continue;
        }

        if (sent < 0 &&
            errno == EINTR)
        {
            continue;
        }

        return false;
    }

    return true;
}

void TcpOuchTransport::close() noexcept
{
    if (socketFd_ >= 0)
    {
        ::close(socketFd_);
        socketFd_ = -1;
    }
}

bool TcpOuchTransport::isConnected() const noexcept
{
    return socketFd_ >= 0;
}

} // namespace ouch
