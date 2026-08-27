#include <benchmark/benchmark.h>

#include "dispatcher/event_dispatcher.hpp"

#include "exchange/exchange_order_session_map.hpp"
#include "exchange/exchange_ouch_handler.hpp"
#include "exchange/exchange_tcp_server.hpp"

#include "execution/ouch/ouch_execution_sink.hpp"
#include "execution/ouch/ouch_messages.hpp"
#include "execution/ouch/ouch_response_dispatcher.hpp"
#include "execution/ouch/tcp_ouch_transport.hpp"

#include "gateway/gateway.hpp"
#include "gateway/gateway_order_intent_sink.hpp"

#include "orderbook/software/array_order_book.hpp"
#include "orderbook/software/matching_engine.hpp"

#include "pipeline/market_data_book_consumer.hpp"
#include "pipeline/market_data_pipeline.hpp"

#include "risk/risk_manager.hpp"

#include "strategy/market_data_strategy_consumer.hpp"
#include "strategy/simple_threshold_strategy.hpp"
#include "strategy/strategy_engine.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <variant>
#include <vector>


namespace
{

constexpr std::size_t EventCount = 500;

constexpr Price StrategyThreshold = 10000;
constexpr Quantity StrategyQuantity = 1;

constexpr AccountId Account = 1001;

constexpr std::size_t AcceptedMessageSize = 64;


MarketDataEvent makeTriggeringEvent(
    std::uint64_t orderId)
{
    MarketDataEvent event{};

    event.type =
        MarketDataEventType::AddOrder;

    event.orderId =
        orderId;

    event.accountId =
        Account;

    event.side =
        Side::Sell;

    event.price =
        StrategyThreshold;

    event.quantity =
        100;

    event.symbol = {
        'A', 'A', 'P', 'L',
        ' ', ' ', ' ', ' '
    };

    return event;
}


void fullEndToEndFlow(
    benchmark::State& state)
{
    for (auto _ : state)
    {
        //
        // Exchange side.
        //
        EventDispatcher exchangeDispatcher;

        //
        // MatchingEngine contains a large
        // ArrayOrderBook internally.
        //
        // Keep it off the benchmark thread's
        // limited stack.
        //
        auto matchingEngine =
            std::make_unique<MatchingEngine>(
                exchangeDispatcher);

        ExchangeOrderSessionMap sessionMap;

        ExchangeOuchHandler exchangeHandler(
            *matchingEngine,
            sessionMap);

        //
        // Port 0 asks the OS to select
        // an available loopback TCP port.
        //
        ExchangeTcpServer exchangeServer(
            0,
            Account,
            exchangeHandler);

        if (!exchangeServer.start())
        {
            state.SkipWithError(
                "Failed to start exchange TCP server");

            break;
        }

        std::atomic<bool> serverRunning{
            true};

        std::atomic<bool> serverFailed{
            false};

        //
        // Exchange epoll thread.
        //
        std::thread serverThread(
            [&]()
            {
                while (serverRunning.load(
                    std::memory_order_acquire))
                {
                    if (!exchangeServer.pollOnce(1))
                    {
                        serverFailed.store(
                            true,
                            std::memory_order_release);

                        break;
                    }
                }
            });

        //
        // Client-side OUCH TCP transport.
        //
        ouch::TcpOuchTransport transport(
            "127.0.0.1",
            exchangeServer.port());

        if (!transport.connect())
        {
            serverRunning.store(
                false,
                std::memory_order_release);

            serverThread.join();

            exchangeServer.stop();

            state.SkipWithError(
                "Failed to connect OUCH TCP transport");

            break;
        }

        //
        // Trading execution path.
        //
        ouch::OuchExecutionSink executionSink(
            transport);

        RiskManager riskManager;

        Gateway gateway(
            riskManager,
            executionSink);

        GatewayOrderIntentSink intentSink(
            gateway);

        //
        // Local reconstructed market-data book.
        //
        // ArrayOrderBook is large, so keep it
        // on the heap as well.
        //
        auto marketBook =
            std::make_unique<ArrayOrderBook>();

        MarketDataBookConsumer bookConsumer(
            *marketBook);

        //
        // Strategy reads the local market view.
        //
        SimpleThresholdStrategy strategy(
            *marketBook,
            Account,
            StrategyThreshold,
            StrategyQuantity);

        StrategyEngine strategyEngine(
            strategy);

        //
        // Market-data consumer:
        //
        // MarketDataEvent
        //      ->
        // Local Order Book
        //      ->
        // Strategy
        //      ->
        // OrderIntent
        //      ->
        // Gateway
        //      ->
        // Risk
        //      ->
        // OUCH
        //
        MarketDataStrategyConsumer consumer(
            bookConsumer,
            strategyEngine,
            intentSink);

        //
        // SPSC market-data pipeline.
        //
        MarketDataPipeline pipeline(
            consumer);

        pipeline.start();

        //
        // Feed market-data events.
        //
        // Every event is a Sell AddOrder at
        // StrategyThreshold, therefore each
        // event should trigger one Buy intent.
        //
        for (std::size_t index = 0;
             index < EventCount;
             ++index)
        {
            const MarketDataEvent event =
                makeTriggeringEvent(
                    100000 + index);

            while (!pipeline.submit(event))
            {
                if (serverFailed.load(
                        std::memory_order_acquire))
                {
                    break;
                }

                std::this_thread::yield();
            }

            if (serverFailed.load(
                    std::memory_order_acquire))
            {
                break;
            }
        }

        //
        // Wait for all market-data events
        // to pass through the consumer path.
        //
        while (
            pipeline.processedCount() <
            EventCount)
        {
            if (serverFailed.load(
                    std::memory_order_acquire))
            {
                break;
            }

            std::this_thread::yield();
        }

        pipeline.stop();

        if (serverFailed.load(
                std::memory_order_acquire))
        {
            transport.close();

            serverRunning.store(
                false,
                std::memory_order_release);

            serverThread.join();

            exchangeServer.stop();

            state.SkipWithError(
                "Exchange TCP server failed");

            break;
        }

        //
        // Every successfully generated EnterOrder
        // should result in one 64-byte Accepted
        // response from the exchange.
        //
        std::size_t acceptedCount = 0;

        std::array<
            std::uint8_t,
            AcceptedMessageSize> response{};

        for (std::size_t index = 0;
             index < EventCount;
             ++index)
        {
            if (!transport.receive(
                    response.data(),
                    response.size()))
            {
                break;
            }

            const auto decoded =
                ouch::OuchResponseDispatcher::dispatch(
                    response.data(),
                    response.size());

            if (!decoded.has_value())
            {
                break;
            }

            if (std::holds_alternative<
                    ouch::Accepted>(
                    *decoded))
            {
                ++acceptedCount;
            }
        }

        benchmark::DoNotOptimize(
            acceptedCount);

        benchmark::DoNotOptimize(
            marketBook->bestAsk());

        benchmark::ClobberMemory();

        //
        // Clean shutdown.
        //
        transport.close();

        serverRunning.store(
            false,
            std::memory_order_release);

        serverThread.join();

        exchangeServer.stop();

        //
        // Verify that every triggering event
        // completed the complete round trip.
        //
        if (acceptedCount !=
            EventCount)
        {
            state.SkipWithError(
                "Not all orders completed the "
                "end-to-end OUCH round trip");

            break;
        }
    }

    state.SetItemsProcessed(
        state.iterations() *
        EventCount);
}


void fullEndToEndLatency(
    benchmark::State& state)
{
    using Clock =
        std::chrono::steady_clock;

    using Nanoseconds =
        std::chrono::nanoseconds;

    //
    // Aggregate latency samples across every
    // Google Benchmark iteration.
    //
    // Each individual benchmark setup still
    // processes only EventCount orders so the
    // production RiskManager limits remain
    // unchanged.
    //
    std::vector<std::uint64_t> allSamples;

    //
    // This is only an initial reservation.
    // Any later growth happens while benchmark
    // timing is paused.
    //
    allSamples.reserve(
        EventCount * 128);

    for (auto _ : state)
    {
        //
        // Setup is deliberately excluded from
        // Google Benchmark timing.
        //
        state.PauseTiming();

        //
        // Exchange side.
        //
        EventDispatcher exchangeDispatcher;

        auto matchingEngine =
            std::make_unique<MatchingEngine>(
                exchangeDispatcher);

        ExchangeOrderSessionMap sessionMap;

        ExchangeOuchHandler exchangeHandler(
            *matchingEngine,
            sessionMap);

        ExchangeTcpServer exchangeServer(
            0,
            Account,
            exchangeHandler);

        if (!exchangeServer.start())
        {
            state.SkipWithError(
                "Failed to start exchange TCP server");

            state.ResumeTiming();
            break;
        }

        std::atomic<bool> serverRunning{
            true};

        std::atomic<bool> serverFailed{
            false};

        //
        // Exchange epoll thread.
        //
        std::thread serverThread(
            [&]()
            {
                while (serverRunning.load(
                    std::memory_order_acquire))
                {
                    if (!exchangeServer.pollOnce(1))
                    {
                        serverFailed.store(
                            true,
                            std::memory_order_release);

                        break;
                    }
                }
            });

        //
        // Client-side OUCH TCP transport.
        //
        ouch::TcpOuchTransport transport(
            "127.0.0.1",
            exchangeServer.port());

        if (!transport.connect())
        {
            serverRunning.store(
                false,
                std::memory_order_release);

            serverThread.join();

            exchangeServer.stop();

            state.SkipWithError(
                "Failed to connect OUCH TCP transport");

            state.ResumeTiming();
            break;
        }

        //
        // Trading execution path.
        //
        ouch::OuchExecutionSink executionSink(
            transport);

        RiskManager riskManager;

        Gateway gateway(
            riskManager,
            executionSink);

        GatewayOrderIntentSink intentSink(
            gateway);

        //
        // Local market-data book.
        //
        auto marketBook =
            std::make_unique<ArrayOrderBook>();

        MarketDataBookConsumer bookConsumer(
            *marketBook);

        //
        // Strategy.
        //
        SimpleThresholdStrategy strategy(
            *marketBook,
            Account,
            StrategyThreshold,
            StrategyQuantity);

        StrategyEngine strategyEngine(
            strategy);

        MarketDataStrategyConsumer consumer(
            bookConsumer,
            strategyEngine,
            intentSink);

        //
        // SPSC market-data pipeline.
        //
        MarketDataPipeline pipeline(
            consumer);

        pipeline.start();

        //
        // Fixed-size per-iteration storage.
        //
        // No latency-sample allocation occurs
        // while an order round trip is measured.
        //
        std::array<
            std::uint64_t,
            EventCount> iterationSamples{};

        std::size_t sampleCount = 0;

        std::array<
            std::uint8_t,
            AcceptedMessageSize> response{};

        std::size_t acceptedCount = 0;

        //
        // Only the sequential trading round
        // trips below contribute to Google
        // Benchmark's measured time.
        //
        state.ResumeTiming();

        //
        // Uncontended end-to-end latency.
        //
        // Submit exactly one market-data event
        // and wait for its OUCH Accepted response
        // before submitting the next event.
        //
        for (std::size_t index = 0;
             index < EventCount;
             ++index)
        {
            const MarketDataEvent event =
                makeTriggeringEvent(
                    200000 + index);

            const auto start =
                Clock::now();

            //
            // MarketDataEvent
            //      ->
            // SPSC
            //      ->
            // Local Order Book
            //      ->
            // Strategy
            //      ->
            // Gateway
            //      ->
            // RiskManager
            //      ->
            // OUCH Encode
            //      ->
            // TCP
            //      ->
            // Exchange / MatchingEngine
            //
            while (!pipeline.submit(event))
            {
                if (serverFailed.load(
                        std::memory_order_acquire))
                {
                    break;
                }

                std::this_thread::yield();
            }

            if (serverFailed.load(
                    std::memory_order_acquire))
            {
                break;
            }

            //
            // Wait for the Accepted response
            // corresponding to this order.
            //
            if (!transport.receive(
                    response.data(),
                    response.size()))
            {
                break;
            }

            const auto decoded =
                ouch::OuchResponseDispatcher::dispatch(
                    response.data(),
                    response.size());

            if (!decoded.has_value())
            {
                break;
            }

            if (!std::holds_alternative<
                    ouch::Accepted>(
                    *decoded))
            {
                break;
            }

            const auto end =
                Clock::now();

            const auto latency =
                std::chrono::duration_cast<
                    Nanoseconds>(
                    end - start)
                    .count();

            iterationSamples[sampleCount] =
                static_cast<std::uint64_t>(
                    latency);

            ++sampleCount;
            ++acceptedCount;
        }

        //
        // Teardown and aggregation must not
        // affect Google Benchmark timing.
        //
        state.PauseTiming();

        pipeline.stop();

        transport.close();

        serverRunning.store(
            false,
            std::memory_order_release);

        serverThread.join();

        exchangeServer.stop();

        if (serverFailed.load(
                std::memory_order_acquire))
        {
            state.SkipWithError(
                "Exchange TCP server failed");

            state.ResumeTiming();
            break;
        }

        if (acceptedCount !=
                EventCount ||
            sampleCount !=
                EventCount)
        {
            state.SkipWithError(
                "Not all latency samples completed "
                "the end-to-end OUCH round trip");

            state.ResumeTiming();
            break;
        }

        //
        // Aggregate the completed iteration's
        // samples while timing is paused.
        //
        allSamples.insert(
            allSamples.end(),
            iterationSamples.begin(),
            iterationSamples.begin() +
                static_cast<std::ptrdiff_t>(
                    sampleCount));

        benchmark::DoNotOptimize(
            marketBook->bestAsk());

        benchmark::ClobberMemory();

        state.ResumeTiming();
    }

    //
    // Calculate percentiles once across every
    // successfully collected latency sample.
    //
    if (!allSamples.empty())
    {
        std::sort(
            allSamples.begin(),
            allSamples.end());

        //
        // Nearest-rank percentile:
        //
        // rank = ceil(p * N)
        // index = rank - 1
        //
        const auto percentile =
            [&](double p)
            {
                const auto rank =
                    static_cast<std::size_t>(
                        std::ceil(
                            p *
                            static_cast<double>(
                                allSamples.size())));

                const auto index =
                    rank == 0
                        ? std::size_t{0}
                        : rank - 1;

                return allSamples[
                    std::min(
                        index,
                        allSamples.size() - 1)];
            };

        state.counters["samples"] =
            static_cast<double>(
                allSamples.size());

        state.counters["p50_ns"] =
            static_cast<double>(
                percentile(0.50));

        state.counters["p95_ns"] =
            static_cast<double>(
                percentile(0.95));

        state.counters["p99_ns"] =
            static_cast<double>(
                percentile(0.99));

        state.counters["p999_ns"] =
            static_cast<double>(
                percentile(0.999));

        state.counters["max_ns"] =
            static_cast<double>(
                allSamples.back());

        benchmark::DoNotOptimize(
            allSamples.data());
    }

    state.SetItemsProcessed(
        state.iterations() *
        EventCount);
}


BENCHMARK(fullEndToEndFlow)
    ->UseRealTime();

BENCHMARK(fullEndToEndLatency)
    ->UseRealTime();

} // namespace