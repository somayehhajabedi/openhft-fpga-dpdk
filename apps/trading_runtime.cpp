#include "execution/ouch/ouch_execution_sink.hpp"
#include "execution/ouch/ouch_transport.hpp"

#include "gateway/gateway.hpp"
#include "gateway/gateway_order_intent_sink.hpp"

#include "market_data/replay/itch_replay_dispatcher.hpp"
#include "market_data/replay/itch_replay_reader.hpp"

#include "orderbook/software/array_order_book.hpp"

#include "pipeline/market_data_book_consumer.hpp"
#include "pipeline/market_data_pipeline.hpp"

#include "risk/risk_manager.hpp"

#include "strategy/market_data_strategy_consumer.hpp"
#include "strategy/simple_threshold_strategy.hpp"
#include "strategy/strategy_engine.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>


namespace
{

class RecordingOuchTransport final
    : public ouch::OuchTransport
{
public:
    bool send(
        const std::uint8_t* data,
        std::size_t length) override
    {
        if (length > buffer_.size())
        {
            return false;
        }

        for (std::size_t index = 0;
             index < length;
             ++index)
        {
            buffer_[index] = data[index];
        }

        lastLength_ = length;
        sent_ = true;

        std::cout
            << "OUCH message generated and sent to transport. "
            << "bytes=" << length
            << '\n';

        return true;
    }

    [[nodiscard]]
    bool sent() const noexcept
    {
        return sent_;
    }

    [[nodiscard]]
    std::size_t lastLength() const noexcept
    {
        return lastLength_;
    }

private:
    std::array<std::uint8_t, 128> buffer_{};

    std::size_t lastLength_{0};

    bool sent_{false};
};


bool parseCpuIndex(
    std::string_view value,
    std::size_t& cpuIndex)
{
    const char* begin =
        value.data();

    const char* end =
        value.data() + value.size();

    const auto [ptr, error] =
        std::from_chars(
            begin,
            end,
            cpuIndex);

    return error == std::errc{} &&
           ptr == end;
}


void printUsage()
{
    std::cerr
        << "Usage:\n"
        << "  trading_runtime <itch_replay_file> "
        << "[--pipeline-cpu <cpu>]\n";
}

} // namespace


int main(
    int argc,
    char** argv)
{
    if (argc < 2)
    {
        printUsage();
        return 1;
    }

    const std::string replayFile =
        argv[1];

    std::optional<std::size_t>
        pipelineCpu;

    for (int index = 2;
         index < argc;
         ++index)
    {
        const std::string_view argument =
            argv[index];

        if (argument == "--pipeline-cpu")
        {
            if (index + 1 >= argc)
            {
                std::cerr
                    << "Missing value for "
                    << "--pipeline-cpu\n";

                printUsage();

                return 1;
            }

            std::size_t cpuIndex = 0;

            if (!parseCpuIndex(
                    argv[index + 1],
                    cpuIndex))
            {
                std::cerr
                    << "Invalid CPU index: "
                    << argv[index + 1]
                    << '\n';

                return 1;
            }

            pipelineCpu =
                cpuIndex;

            ++index;

            continue;
        }

        std::cerr
            << "Unknown argument: "
            << argument
            << '\n';

        printUsage();

        return 1;
    }

    if (pipelineCpu.has_value())
    {
        std::cout
            << "Pipeline CPU affinity requested: CPU "
            << pipelineCpu.value()
            << '\n';
    }
    else
    {
        std::cout
            << "Pipeline CPU affinity: scheduler managed\n";
    }

    //
    // Local reconstructed market-data order book.
    //
    ArrayOrderBook marketBook;

    //
    // Writes market-data events into marketBook.
    //
    MarketDataBookConsumer bookConsumer(
        marketBook);

    //
    // Strategy reads the same marketBook
    // through the read-only MarketView interface.
    //
    SimpleThresholdStrategy strategy(
        marketBook,
        1001,
        100,
        10);

    StrategyEngine strategyEngine(
        strategy);

    //
    // Execution path.
    //
    RecordingOuchTransport transport;

    ouch::OuchExecutionSink executionSink(
        transport);

    RiskManager riskManager;

    Gateway gateway(
        riskManager,
        executionSink);

    GatewayOrderIntentSink intentSink(
        gateway);

    //
    // Market-data consumer:
    //
    // 1. update book
    // 2. run strategy
    // 3. forward generated intent to Gateway
    //
    MarketDataStrategyConsumer consumer(
        bookConsumer,
        strategyEngine,
        intentSink);

    //
    // SPSC market-data pipeline.
    //
    MarketDataPipeline pipeline(
        consumer,
        pipelineCpu);

    //
    // Replay input.
    //
    ItchReplayReader reader(
        replayFile);

    if (!reader.isOpen())
    {
        std::cerr
            << "Failed to open replay file: "
            << replayFile
            << '\n';

        return 1;
    }

    //
    // Raw ITCH -> MarketDataEvent -> pipeline.
    //
    ItchReplayDispatcher replayDispatcher(
        pipeline);

    pipeline.start();

    std::vector<std::uint8_t> message;

    std::size_t replayedMessages = 0;

    while (reader.readNext(message))
    {
        if (replayDispatcher.dispatch(
                message.data(),
                message.size()))
        {
            ++replayedMessages;
        }
    }

    while (pipeline.processedCount() <
           replayedMessages)
    {
        std::this_thread::yield();
    }

    pipeline.stop();

    std::cout
        << "Replay complete. "
        << replayedMessages
        << " market-data messages processed.\n";

    const PriceLevel* bestBid =
        marketBook.bestBid();

    const PriceLevel* bestAsk =
        marketBook.bestAsk();

    if (bestBid != nullptr)
    {
        std::cout
            << "Best Bid: "
            << bestBid->price
            << '\n';
    }

    if (bestAsk != nullptr)
    {
        std::cout
            << "Best Ask: "
            << bestAsk->price
            << '\n';
    }

    if (transport.sent())
    {
        std::cout
            << "Execution path reached OUCH transport. "
            << "last message bytes="
            << transport.lastLength()
            << '\n';
    }

    return 0;
}