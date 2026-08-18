#include "market_data/replay/itch_replay_dispatcher.hpp"
#include "market_data/replay/itch_replay_reader.hpp"

#include "orderbook/software/array_order_book.hpp"

#include "pipeline/market_data_book_consumer.hpp"
#include "pipeline/market_data_pipeline.hpp"

#include "strategy/market_data_strategy_consumer.hpp"
#include "strategy/order_intent_sink.hpp"
#include "strategy/simple_threshold_strategy.hpp"
#include "strategy/strategy_engine.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <thread>


namespace
{

class LoggingOrderIntentSink final
    : public OrderIntentSink
{
public:
    bool submit(
        const OrderIntent& intent) override
    {
        std::cout
            << "OrderIntent generated: "
            << "price=" << intent.price
            << " quantity=" << intent.quantity
            << '\n';

        return true;
    }
};

} // namespace


int main(
    int argc,
    char** argv)
{
    if (argc != 2)
    {
        std::cerr
            << "Usage: trading_runtime <itch_replay_file>\n";

        return 1;
    }

    const std::string replayFile =
        argv[1];

    //
    // Local reconstructed market-data order book.
    //
    ArrayOrderBook marketBook;

    //
    // Writes market-data events into the book.
    //
    MarketDataBookConsumer bookConsumer(
        marketBook);

    //
    // Reads the same book through MarketView.
    //
    SimpleThresholdStrategy strategy(
        marketBook,
        1001,
        100,
        10);

    StrategyEngine strategyEngine(
        strategy);

    LoggingOrderIntentSink intentSink;

    MarketDataStrategyConsumer consumer(
        bookConsumer,
        strategyEngine,
        intentSink);

    //
    // SPSC market-data pipeline.
    //
    MarketDataPipeline pipeline(
        consumer);

    //
    // Replay source.
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
    // Converts raw ITCH messages into MarketDataEvent
    // and submits them to the pipeline.
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

    //
    // Wait until the consumer thread has processed
    // all successfully submitted events.
    //
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

    return 0;
}
