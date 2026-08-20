#include "exchange/exchange_tcp_server.hpp"

#include "logging/async_logger.hpp"

#include <spdlog/spdlog.h>

#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <utility>

namespace
{

template <typename... Args>
void logInfo(
    spdlog::format_string_t<Args...> format,
    Args&&... args)
{
    if (auto logger = AsyncLogger::get())
    {
        logger->info(
            format,
            std::forward<Args>(args)...);
    }
}

template <typename... Args>
void logError(
    spdlog::format_string_t<Args...> format,
    Args&&... args)
{
    if (auto logger = AsyncLogger::get())
    {
        logger->error(
            format,
            std::forward<Args>(args)...);
    }
}

} // namespace

ExchangeTcpServer::ExchangeTcpServer(
    std::uint16_t port,
    AccountId accountId,
    ExchangeOuchHandler& handler)
    :
    port_(port),
    accountId_(accountId),
    handler_(handler)
{
}

ExchangeTcpServer::~ExchangeTcpServer()
{
    stop();
}

bool ExchangeTcpServer::start()
{
    listenFd_ =
        ::socket(
            AF_INET,
            SOCK_STREAM,
            0);

    if (listenFd_ < 0)
    {
        logError(
            "socket creation failed errno={}",
            errno);

        return false;
    }

    int reuse = 1;

    ::setsockopt(
        listenFd_,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse));

    if (!setNonBlocking(
            listenFd_))
    {
        logError(
            "failed to set listening socket non-blocking fd={} errno={}",
            listenFd_,
            errno);

        stop();
        return false;
    }

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr =
        htonl(INADDR_LOOPBACK);
    address.sin_port =
        htons(port_);

    if (::bind(
            listenFd_,
            reinterpret_cast<sockaddr*>(
                &address),
            sizeof(address)) < 0)
    {
        logError(
            "bind failed port={} errno={}",
            port_,
            errno);

        stop();
        return false;
    }

    if (port_ == 0)
    {
        sockaddr_in boundAddress{};

        socklen_t boundAddressLength =
            sizeof(boundAddress);

        if (::getsockname(
                listenFd_,
                reinterpret_cast<sockaddr*>(
                    &boundAddress),
                &boundAddressLength) < 0)
        {
            logError(
                "getsockname failed errno={}",
                errno);

            stop();
            return false;
        }

        port_ =
            ntohs(
                boundAddress.sin_port);
    }

    if (::listen(
            listenFd_,
            SOMAXCONN) < 0)
    {
        logError(
            "listen failed port={} errno={}",
            port_,
            errno);

        stop();
        return false;
    }

    epollFd_ =
        ::epoll_create1(
            EPOLL_CLOEXEC);

    if (epollFd_ < 0)
    {
        logError(
            "epoll_create1 failed errno={}",
            errno);

        stop();
        return false;
    }

    epoll_event event{};

    event.events =
        EPOLLIN;

    event.data.fd =
        listenFd_;

    if (::epoll_ctl(
            epollFd_,
            EPOLL_CTL_ADD,
            listenFd_,
            &event) < 0)
    {
        logError(
            "epoll_ctl add listen socket failed fd={} errno={}",
            listenFd_,
            errno);

        stop();
        return false;
    }

    logInfo(
        "Exchange TCP server started port={}",
        port_);

    return true;
}

bool ExchangeTcpServer::pollOnce(
    int timeoutMilliseconds)
{
    if (epollFd_ < 0)
    {
        return false;
    }

    std::array<
        epoll_event,
        MaxEvents> events{};

    const int eventCount =
        ::epoll_wait(
            epollFd_,
            events.data(),
            static_cast<int>(
                events.size()),
            timeoutMilliseconds);

    if (eventCount < 0)
    {
        if (errno == EINTR)
        {
            return true;
        }

        logError(
            "epoll_wait failed errno={}",
            errno);

        return false;
    }

    for (int index = 0;
         index < eventCount;
         ++index)
    {
        const int fd =
            events[index].data.fd;

        if (fd == listenFd_)
        {
            if (!acceptClients())
            {
                return false;
            }

            continue;
        }

	if ((events[index].events &
             (EPOLLERR | EPOLLHUP)) != 0)
        {
            const std::uint32_t eventMask =
               events[index].events;

            logError(
                "epoll client error fd={} events={}",
                fd,
                eventMask);

            closeClient(fd);
            continue;
         }


        if ((events[index].events &
             EPOLLIN) != 0)
        {
            if (!handleClientReadable(
                    fd))
            {
                closeClient(fd);
            }
        }
    }

    return true;
}

bool ExchangeTcpServer::setNonBlocking(
    int fd)
{
    const int flags =
        ::fcntl(
            fd,
            F_GETFL,
            0);

    if (flags < 0)
    {
        return false;
    }

    return
        ::fcntl(
            fd,
            F_SETFL,
            flags | O_NONBLOCK) == 0;
}

bool ExchangeTcpServer::acceptClients()
{
    while (true)
    {
        const int clientFd =
            ::accept4(
                listenFd_,
                nullptr,
                nullptr,
                SOCK_NONBLOCK |
                SOCK_CLOEXEC);

        if (clientFd >= 0)
        {
            epoll_event event{};

            event.events =
                EPOLLIN;

            event.data.fd =
                clientFd;

            if (::epoll_ctl(
                    epollFd_,
                    EPOLL_CTL_ADD,
                    clientFd,
                    &event) < 0)
            {
                logError(
                    "epoll_ctl add client failed fd={} errno={}",
                    clientFd,
                    errno);

                ::close(clientFd);
                return false;
            }

            clients_.emplace(
                clientFd,
                ClientState{});

            logInfo(
                "TCP client connected fd={}",
                clientFd);

            continue;
        }

        if (errno == EAGAIN ||
            errno == EWOULDBLOCK)
        {
            return true;
        }

        if (errno == EINTR)
        {
            continue;
        }

        logError(
            "accept4 failed errno={}",
            errno);

        return false;
    }
}

bool ExchangeTcpServer::handleClientReadable(
    int clientFd)
{
    auto it =
        clients_.find(
            clientFd);

    if (it == clients_.end())
    {
        logError(
            "client state not found fd={}",
            clientFd);

        return false;
    }

    ClientState& state =
        it->second;

    while (true)
    {
        const std::size_t remaining =
            EnterOrderSize -
            state.receivedBytes;

        const ssize_t received =
            ::recv(
                clientFd,
                state.receiveBuffer.data() +
                    state.receivedBytes,
                remaining,
                0);

        if (received > 0)
        {
            state.receivedBytes +=
                static_cast<std::size_t>(
                    received);

            if (state.receivedBytes ==
                EnterOrderSize)
            {
                const auto response =
                    handler_.handleEnterOrder(
                        accountId_,
                        state.receiveBuffer.data(),
                        state.receiveBuffer.size());

                if (!response.has_value())
                {
                    logError(
                        "OUCH EnterOrder handling failed fd={}",
                        clientFd);

                    return false;
                }

                if (!sendAll(
                        clientFd,
                        response->data(),
                        response->size()))
                {
                    return false;
                }

                state.receivedBytes = 0;
            }

            continue;
        }

        if (received == 0)
        {
            logInfo(
                "TCP peer closed connection fd={}",
                clientFd);

            return false;
        }

        if (errno == EAGAIN ||
            errno == EWOULDBLOCK)
        {
            return true;
        }

        if (errno == EINTR)
        {
            continue;
        }

        logError(
            "recv failed fd={} errno={}",
            clientFd,
            errno);

        return false;
    }
}

bool ExchangeTcpServer::sendAll(
    int clientFd,
    const std::uint8_t* data,
    std::size_t length)
{
    std::size_t sentTotal = 0;

    while (sentTotal < length)
    {
        const ssize_t sent =
            ::send(
                clientFd,
                data + sentTotal,
                length - sentTotal,
                MSG_NOSIGNAL);

        if (sent > 0)
        {
            sentTotal +=
                static_cast<std::size_t>(
                    sent);

            continue;
        }

        if (sent < 0 &&
            errno == EINTR)
        {
            continue;
        }

        if (sent < 0 &&
            (errno == EAGAIN ||
             errno == EWOULDBLOCK))
        {
            continue;
        }

        logError(
            "send failed fd={} errno={}",
            clientFd,
            errno);

        return false;
    }

    return true;
}

void ExchangeTcpServer::closeClient(
    int clientFd)
{
    logInfo(
        "TCP client disconnected fd={}",
        clientFd);

    ::epoll_ctl(
        epollFd_,
        EPOLL_CTL_DEL,
        clientFd,
        nullptr);

    ::close(
        clientFd);

    clients_.erase(
        clientFd);
}

void ExchangeTcpServer::stop() noexcept
{
    for (const auto& entry :
         clients_)
    {
        ::close(
            entry.first);
    }

    clients_.clear();

    if (epollFd_ >= 0)
    {
        ::close(
            epollFd_);

        epollFd_ = -1;
    }

    if (listenFd_ >= 0)
    {
        ::close(
            listenFd_);

        listenFd_ = -1;
    }
}

std::uint16_t
ExchangeTcpServer::port() const noexcept
{
    return port_;
}
