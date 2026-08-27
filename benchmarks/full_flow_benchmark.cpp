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

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <variant>


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


BENCHMARK(fullEndToEndFlow)
    ->UseRealTime();

} // namespace
