/*
 * Exchange Runtime
 * ----------------
 * Standalone exchange simulator runtime.
 *
 * Responsibilities:
 * - Construct the exchange-side matching engine stack.
 * - Start ExchangeTcpServer in TLS mode.
 * - Run the epoll event loop continuously.
 *
 * This executable exists so trading_runtime can connect to a
 * separate process over TLS, which better reflects a production-like
 * client/exchange architecture than embedding both sides together.
 */

#include "dispatcher/event_dispatcher.hpp"

#include "exchange/exchange_order_session_map.hpp"
#include "exchange/exchange_ouch_handler.hpp"
#include "exchange/exchange_tcp_server.hpp"

#include "orderbook/software/matching_engine.hpp"

#include <atomic>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>


namespace
{

bool parsePort(
    std::string_view value,
    std::uint16_t& port)
{
    unsigned int parsedPort = 0;

    const char* begin =
        value.data();

    const char* end =
        value.data() + value.size();

    const auto [ptr, error] =
        std::from_chars(
            begin,
            end,
            parsedPort);

    if (error != std::errc{} ||
        ptr != end ||
        parsedPort > 65535)
    {
        return false;
    }

    port =
        static_cast<std::uint16_t>(
            parsedPort);

    return true;
}


void printUsage()
{
    std::cerr
        << "Usage:\n"
        << "  exchange_runtime "
        << "--port <port> "
        << "--cert <certificate> "
        << "--key <private-key>\n";
}

} // namespace


int main(
    int argc,
    char** argv)
{
    std::uint16_t port = 0;

    std::string certificatePath;

    std::string privateKeyPath;


    for (int index = 1;
         index < argc;
         ++index)
    {
        const std::string_view argument =
            argv[index];


        if (argument == "--port")
        {
            if (index + 1 >= argc)
            {
                std::cerr
                    << "Missing value for --port\n";

                printUsage();

                return 1;
            }

            if (!parsePort(
                    argv[index + 1],
                    port))
            {
                std::cerr
                    << "Invalid port: "
                    << argv[index + 1]
                    << '\n';

                return 1;
            }

            ++index;

            continue;
        }


        if (argument == "--cert")
        {
            if (index + 1 >= argc)
            {
                std::cerr
                    << "Missing value for --cert\n";

                return 1;
            }

            certificatePath =
                argv[++index];

            continue;
        }


        if (argument == "--key")
        {
            if (index + 1 >= argc)
            {
                std::cerr
                    << "Missing value for --key\n";

                return 1;
            }

            privateKeyPath =
                argv[++index];

            continue;
        }


        std::cerr
            << "Unknown argument: "
            << argument
            << '\n';

        printUsage();

        return 1;
    }


    if (certificatePath.empty())
    {
        std::cerr
            << "TLS certificate is required\n";

        return 1;
    }


    if (privateKeyPath.empty())
    {
        std::cerr
            << "TLS private key is required\n";

        return 1;
    }


    EventDispatcher dispatcher;


    MatchingEngine matchingEngine(
        dispatcher);


    ExchangeOrderSessionMap sessionMap;


    ExchangeOuchHandler handler(
        matchingEngine,
        sessionMap);


    ExchangeTcpServer server(
        port,
        1001,
        handler,
        certificatePath,
        privateKeyPath);


    if (!server.start())
    {
        std::cerr
            << "Failed to start exchange server\n";

        return 1;
    }


    std::cout
        << "Exchange TLS server started\n";

    std::cout
        << "Listening on 127.0.0.1:"
        << server.port()
        << '\n';


    while (true)
    {
        if (!server.pollOnce(100))
        {
            std::cerr
                << "Exchange event loop failed\n";

            break;
        }
    }


    server.stop();

    return 0;
}
