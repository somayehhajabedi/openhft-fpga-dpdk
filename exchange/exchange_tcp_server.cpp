#include "exchange/exchange_tcp_server.hpp"

#include "logging/async_logger.hpp"

#include <spdlog/spdlog.h>
#include <openssl/ssl.h>

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
    ExchangeOuchHandler& handler,
    std::string certificatePath,
    std::string privateKeyPath)
    :
    port_(port),
    accountId_(accountId),
    handler_(handler),
    certificatePath_(
        std::move(certificatePath)),
    privateKeyPath_(
        std::move(privateKeyPath))
{
}

ExchangeTcpServer::~ExchangeTcpServer()
{
    stop();
}

bool ExchangeTcpServer::initializeTls()
{
    if (certificatePath_.empty() ||
        privateKeyPath_.empty())
    {
        return true;
    }

    sslContext_ =
        SSL_CTX_new(
            TLS_server_method());

    if (sslContext_ == nullptr)
    {
        logError(
            "SSL_CTX_new failed");

        return false;
    }

    if (SSL_CTX_use_certificate_file(
            sslContext_,
            certificatePath_.c_str(),
            SSL_FILETYPE_PEM) != 1)
    {
        logError(
            "failed to load TLS certificate path={}",
            certificatePath_);

        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(
            sslContext_,
            privateKeyPath_.c_str(),
            SSL_FILETYPE_PEM) != 1)
    {
        logError(
            "failed to load TLS private key path={}",
            privateKeyPath_);

        return false;
    }

    if (SSL_CTX_check_private_key(
            sslContext_) != 1)
    {
        logError(
            "TLS certificate/private key mismatch");

        return false;
    }

    return true;
}

bool ExchangeTcpServer::updateClientEvents(
    int clientFd,
    std::uint32_t events)
{
    epoll_event event{};

    event.events = events;
    event.data.fd = clientFd;

    return
        ::epoll_ctl(
            epollFd_,
            EPOLL_CTL_MOD,
            clientFd,
            &event) == 0;
}


bool ExchangeTcpServer::start()
{

    if (!initializeTls())
    {
        stop();
        return false;
    }


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


    auto clientIt =
    clients_.find(fd);

    if (clientIt != clients_.end() &&
        !clientIt->second.tlsHandshakeComplete)
    {
        if (!progressTlsHandshake(fd))
        {
            closeClient(fd);
        }

        continue;
    }


    if ((events[index].events &
     EPOLLOUT) != 0)
    {
        auto it =
            clients_.find(fd);

        if (it != clients_.end() &&
            !it->second.sendBuffer.empty())
        {
            if (!flushPendingWrite(fd))
            {
                closeClient(fd);
                continue;
            }
        }
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

           ClientState state{};

            if (sslContext_ != nullptr)
            {
                state.ssl =
                    SSL_new(
                        sslContext_);

                if (state.ssl == nullptr)
                {
                    logError(
                        "SSL_new failed fd={}",
                        clientFd);

                    ::close(clientFd);
                    return false;
                }

                if (SSL_set_fd(
                        state.ssl,
                        clientFd) != 1)
                {
                    logError(
                        "SSL_set_fd failed fd={}",
                        clientFd);

                    SSL_free(state.ssl);
                    ::close(clientFd);
                    return false;
                }

                SSL_set_accept_state(
                    state.ssl);
            }
            else
            {
                state.tlsHandshakeComplete = true;
            }

            clients_.emplace(
                clientFd,
                state);

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


bool ExchangeTcpServer::progressTlsHandshake(
    int clientFd)
{
    auto it =
        clients_.find(
            clientFd);

    if (it == clients_.end())
    {
        return false;
    }

    ClientState& state =
        it->second;

    if (state.tlsHandshakeComplete)
    {
        return true;
    }

    if (state.ssl == nullptr)
    {
        return false;
    }

    const int result =
        SSL_accept(
            state.ssl);

    if (result == 1)
    {
        state.tlsHandshakeComplete = true;

        if (!updateClientEvents(
                clientFd,
                EPOLLIN))
        {
            return false;
        }

        logInfo(
            "TLS handshake completed fd={}",
            clientFd);

        return true;
    }

    const int error =
        SSL_get_error(
            state.ssl,
            result);

    if (error ==
        SSL_ERROR_WANT_READ)
    {
        return
            updateClientEvents(
                clientFd,
                EPOLLIN);
    }

    if (error ==
        SSL_ERROR_WANT_WRITE)
    {
        return
            updateClientEvents(
                clientFd,
                EPOLLIN |
                EPOLLOUT);
    }

    logError(
        "TLS handshake failed fd={} ssl_error={}",
        clientFd,
        error);

    return false;
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

        int received = 0;

        if (state.ssl != nullptr)
        {
            received =
                SSL_read(
                    state.ssl,
                    state.receiveBuffer.data() +
                        state.receivedBytes,
                    static_cast<int>(
                        remaining));
        }
        else
        {
            received =
                static_cast<int>(
                    ::recv(
                        clientFd,
                        state.receiveBuffer.data() +
                            state.receivedBytes,
                        remaining,
                        0));
        }

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

        if (state.ssl != nullptr)
        {
            const int error =
                SSL_get_error(
                    state.ssl,
                    received);

            if (error ==
                SSL_ERROR_WANT_READ)
            {
                return
                    updateClientEvents(
                        clientFd,
                        EPOLLIN |
                        (state.sendBuffer.empty()
                             ? 0U
                             : EPOLLOUT));
            }

            if (error ==
                SSL_ERROR_WANT_WRITE)
            {
                return
                    updateClientEvents(
                        clientFd,
                        EPOLLIN |
                        EPOLLOUT);
            }

            if (error ==
                    SSL_ERROR_ZERO_RETURN)
            {
                logInfo(
                    "TLS peer closed connection fd={}",
                    clientFd);

                return false;
            }

            logError(
                "SSL_read failed fd={} ssl_error={}",
                clientFd,
                error);

            return false;
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
    auto it =
        clients_.find(
            clientFd);

    if (it == clients_.end())
    {
        return false;
    }

    ClientState& state =
        it->second;

    if (!state.sendBuffer.empty())
    {
        return false;
    }

    state.sendBuffer.assign(
        data,
        data + length);

    state.sentBytes = 0;

    return
        flushPendingWrite(
            clientFd);
}



bool ExchangeTcpServer::flushPendingWrite(
    int clientFd)
{
    auto it =
        clients_.find(
            clientFd);

    if (it == clients_.end())
    {
        return false;
    }

    ClientState& state =
        it->second;

    while (state.sentBytes <
           state.sendBuffer.size())
    {
        const std::size_t remaining =
            state.sendBuffer.size() -
            state.sentBytes;

        int sent = 0;

        if (state.ssl != nullptr)
        {
            sent =
                SSL_write(
                    state.ssl,
                    state.sendBuffer.data() +
                        state.sentBytes,
                    static_cast<int>(
                        remaining));
        }
        else
        {
            sent =
                static_cast<int>(
                    ::send(
                        clientFd,
                        state.sendBuffer.data() +
                            state.sentBytes,
                        remaining,
                        MSG_NOSIGNAL));
        }

        if (sent > 0)
        {
            state.sentBytes +=
                static_cast<std::size_t>(
                    sent);

            continue;
        }

        if (state.ssl != nullptr)
        {
            const int error =
                SSL_get_error(
                    state.ssl,
                    sent);

            if (error ==
                SSL_ERROR_WANT_WRITE)
            {
                return
                    updateClientEvents(
                        clientFd,
                        EPOLLIN |
                        EPOLLOUT);
            }

            if (error ==
                SSL_ERROR_WANT_READ)
            {
                return
                    updateClientEvents(
                        clientFd,
                        EPOLLIN |
                        EPOLLOUT);
            }

            logError(
                "SSL_write failed fd={} ssl_error={}",
                clientFd,
                error);

            return false;
        }

        if (errno == EINTR)
        {
            continue;
        }

        if (errno == EAGAIN ||
            errno == EWOULDBLOCK)
        {
            return
                updateClientEvents(
                    clientFd,
                    EPOLLIN |
                    EPOLLOUT);
        }

        logError(
            "send failed fd={} errno={}",
            clientFd,
            errno);

        return false;
    }

    state.sendBuffer.clear();
    state.sentBytes = 0;

    return
        updateClientEvents(
            clientFd,
            EPOLLIN);
}


void ExchangeTcpServer::closeClient(
    int clientFd)
{
    logInfo(
        "TCP client disconnected fd={}",
        clientFd);

    auto it =
        clients_.find(
            clientFd);

    if (it != clients_.end())
    {
        if (it->second.ssl != nullptr)
        {
            SSL_shutdown(
                it->second.ssl);

            SSL_free(
                it->second.ssl);

            it->second.ssl = nullptr;
        }
    }

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
    for (auto& entry :
         clients_)
    {
        if (entry.second.ssl != nullptr)
        {
            SSL_shutdown(
                entry.second.ssl);

            SSL_free(
                entry.second.ssl);

            entry.second.ssl = nullptr;
        }

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

    if (sslContext_ != nullptr)
    {
        SSL_CTX_free(
            sslContext_);

        sslContext_ = nullptr;
    }
}

std::uint16_t
ExchangeTcpServer::port() const noexcept
{
    return port_;
}
