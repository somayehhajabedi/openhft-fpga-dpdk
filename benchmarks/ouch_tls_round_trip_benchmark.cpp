/*
 * OUCH TLS Round-Trip Latency Benchmark
 * -------------------------------------
 * Measures steady-state application-level round-trip latency for an
 * OUCH EnterOrder traveling from the client through TLS to the exchange
 * simulator and returning as an OUCH Accepted response.
 *
 * The TLS handshake is intentionally excluded from the timed region.
 *
 * Orders alternate between Buy and Sell at the same price and quantity.
 * This causes consecutive orders to match, preventing the exchange
 * order book/order pool from filling during long benchmark runs.
 */

#include "dispatcher/event_dispatcher.hpp"
#include "exchange/exchange_ouch_handler.hpp"
#include "exchange/exchange_tcp_server.hpp"
#include "execution/ouch/accepted_encoder.hpp"
#include "execution/ouch/ouch_encoder.hpp"
#include "execution/ouch/ouch_messages.hpp"
#include "execution/ouch/ouch_response_dispatcher.hpp"
#include "execution/ouch/tls_ouch_transport.hpp"
#include "orderbook/software/matching_engine.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <variant>
#include <vector>

namespace
{

using Clock =
    std::chrono::steady_clock;

using Nanoseconds =
    std::chrono::nanoseconds;

constexpr std::size_t WarmupIterations =
    1'000;

constexpr std::size_t BenchmarkIterations =
    100'000;

double percentile(
    const std::vector<std::int64_t>& samples,
    double p)
{
    if (samples.empty())
    {
        return 0.0;
    }

    const double position =
        p *
        static_cast<double>(
            samples.size() - 1);

    const auto index =
        static_cast<std::size_t>(
            position);

    return static_cast<double>(
        samples[index]);
}

bool performRoundTrip(
    ouch::TlsOuchTransport& transport,
    const ouch::EnterOrder& order)
{
    const auto request =
        ouch::OuchEncoder::encode(
            order);

    if (!transport.send(
            request.data(),
            request.size()))
    {
        return false;
    }

    std::array<
        std::uint8_t,
        ouch::AcceptedEncoder::AcceptedSize>
        response{};

    if (!transport.receive(
            response.data(),
            response.size()))
    {
        return false;
    }

    const auto dispatched =
        ouch::OuchResponseDispatcher::dispatch(
            response.data(),
            response.size());

    if (!dispatched.has_value())
    {
        return false;
    }

    if (!std::holds_alternative<
            ouch::Accepted>(
                *dispatched))
    {
        return false;
    }

    const auto& accepted =
        std::get<
            ouch::Accepted>(
                *dispatched);

    return accepted.userRefNum ==
        order.userRefNum;
}

} // namespace

int main(
    int argc,
    char* argv[])
{
    if (argc != 3)
    {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <server.crt> <server.key>\n";

        return EXIT_FAILURE;
    }

    const std::string certificatePath =
        argv[1];

    const std::string privateKeyPath =
        argv[2];

    //
    // Build the exchange side of the benchmark.
    //
    EventDispatcher dispatcher;

    MatchingEngine matchingEngine(
        dispatcher);

    ExchangeOrderSessionMap sessionMap;

    ExchangeOuchHandler handler(
        matchingEngine,
        sessionMap);

    //
    // Port 0 asks the operating system to select
    // an available loopback port.
    //
    ExchangeTcpServer server(
        0,
        1001,
        handler,
        certificatePath,
        privateKeyPath);

    if (!server.start())
    {
        std::cerr
            << "Failed to start exchange server\n";

        return EXIT_FAILURE;
    }

    std::atomic<bool> running{
        true};

    std::atomic<bool> serverOk{
        true};

    //
    // Exchange event-loop thread.
    //
    // epoll itself does not create a thread.
    // This thread repeatedly drives the server reactor.
    //
    std::thread serverThread(
        [&]()
        {
            while (running.load(
                std::memory_order_relaxed))
            {
                if (!server.pollOnce(1))
                {
                    serverOk.store(
                        false,
                        std::memory_order_relaxed);

                    break;
                }
            }
        });

    //
    // Establish TLS once.
    //
    // TLS handshake latency is deliberately not part
    // of the steady-state order round-trip measurement.
    //
    ouch::TlsOuchTransport transport(
        "127.0.0.1",
        server.port(),
        certificatePath);

    if (!transport.connect())
    {
        running.store(
            false,
            std::memory_order_relaxed);

        serverThread.join();

        server.stop();

        std::cerr
            << "TLS connection failed\n";

        return EXIT_FAILURE;
    }

    ouch::EnterOrder order{};

    order.userRefNum =
        1;

    order.side =
        Side::Buy;

    order.quantity =
        25;

    order.symbol = {
        'A', 'A', 'P', 'L',
        ' ', ' ', ' ', ' '
    };

    order.price =
        101;

    //
    // Warmup.
    //
    // Consecutive orders alternate:
    //
    // Buy  25 @ 101
    // Sell 25 @ 101
    //
    // Therefore the Sell matches the preceding Buy
    // and the order book does not continuously grow.
    //
    for (std::size_t i = 0;
         i < WarmupIterations;
         ++i)
    {
        order.userRefNum =
            static_cast<
                ouch::UserRefNum>(
                    i + 1);

        order.side =
            (i % 2 == 0)
                ? Side::Buy
                : Side::Sell;

        if (!performRoundTrip(
                transport,
                order))
        {
            std::cerr
                << "Warmup round trip failed at iteration "
                << i
                << ", userRefNum="
                << order.userRefNum
                << '\n';

            running.store(
                false,
                std::memory_order_relaxed);

            serverThread.join();

            server.stop();

            return EXIT_FAILURE;
        }
    }

    std::vector<std::int64_t>
        latencySamples;

    latencySamples.reserve(
        BenchmarkIterations);

    //
    // Benchmark.
    //
    const auto benchmarkStart =
        Clock::now();

    for (std::size_t i = 0;
         i < BenchmarkIterations;
         ++i)
    {
        order.userRefNum =
            static_cast<
                ouch::UserRefNum>(
                    WarmupIterations +
                    i +
                    1);

        //
        // Alternate Buy/Sell so every pair matches.
        //
        // This prevents accumulation of live orders
        // in the matching engine.
        //
        order.side =
            (i % 2 == 0)
                ? Side::Buy
                : Side::Sell;

        const auto start =
            Clock::now();

        if (!performRoundTrip(
                transport,
                order))
        {
            std::cerr
                << "Benchmark round trip failed at iteration "
                << i
                << ", userRefNum="
                << order.userRefNum
                << '\n';

            running.store(
                false,
                std::memory_order_relaxed);

            serverThread.join();

            server.stop();

            return EXIT_FAILURE;
        }

        const auto end =
            Clock::now();

        latencySamples.push_back(
            std::chrono::duration_cast<
                Nanoseconds>(
                    end - start)
                .count());
    }

    const auto benchmarkEnd =
        Clock::now();

    //
    // Shutdown.
    //
    running.store(
        false,
        std::memory_order_relaxed);

    serverThread.join();

    transport.close();

    server.stop();

    if (!serverOk.load(
            std::memory_order_relaxed))
    {
        std::cerr
            << "Exchange server failed during benchmark\n";

        return EXIT_FAILURE;
    }

    //
    // Calculate latency distribution.
    //
    std::sort(
        latencySamples.begin(),
        latencySamples.end());

    const auto totalLatency =
        std::accumulate(
            latencySamples.begin(),
            latencySamples.end(),
            std::int64_t{0});

    const double mean =
        static_cast<double>(
            totalLatency) /
        static_cast<double>(
            latencySamples.size());

    const auto elapsed =
        std::chrono::duration<double>(
            benchmarkEnd -
            benchmarkStart)
            .count();

    const double throughput =
        static_cast<double>(
            BenchmarkIterations) /
        elapsed;

    //
    // Report.
    //
    std::cout
        << "\nOUCH TLS Round-Trip Benchmark\n"
        << "-----------------------------\n";

    std::cout
        << "Samples:    "
        << latencySamples.size()
        << '\n';

    std::cout
        << "Min:        "
        << latencySamples.front()
        << " ns\n";

    std::cout
        << "Mean:       "
        << mean
        << " ns\n";

    std::cout
        << "p50:        "
        << percentile(
            latencySamples,
            0.50)
        << " ns\n";

    std::cout
        << "p95:        "
        << percentile(
            latencySamples,
            0.95)
        << " ns\n";

    std::cout
        << "p99:        "
        << percentile(
            latencySamples,
            0.99)
        << " ns\n";

    std::cout
        << "p99.9:      "
        << percentile(
            latencySamples,
            0.999)
        << " ns\n";

    std::cout
        << "Max:        "
        << latencySamples.back()
        << " ns\n";

    std::cout
        << "Throughput: "
        << throughput
        << " round-trips/sec\n";

    return EXIT_SUCCESS;
}