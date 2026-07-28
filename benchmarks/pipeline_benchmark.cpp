#include <benchmark/benchmark.h>

#include <arpa/inet.h>
#include <endian.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "dpdk/parser/itch/messages/add_order.hpp"
#include "dpdk/parser/itch/messages/order_cancel.hpp"
#include "dpdk/parser/itch/messages/order_delete.hpp"
#include "dpdk/parser/itch/messages/order_executed.hpp"
#include "dpdk/parser/itch/messages/order_replace.hpp"

#include "gateway/market_data/itch_handler.hpp"
#include "market_data/replay/itch_replay_dispatcher.hpp"
#include "market_data/replay/itch_replay_reader.hpp"
#include "orderbook/software/array_order_book.hpp"

namespace
{

constexpr std::size_t SequenceCount = 1000;
constexpr std::size_t MessagesPerSequence = 5;

const std::string ReplayPath{
    "pipeline_benchmark_replay.bin"
};

template <typename Message>
void writeMessage(
    std::ofstream& stream,
    const Message& message)
{
    const auto messageLength =
        static_cast<std::uint16_t>(sizeof(Message));

    const std::uint16_t networkLength =
        htons(messageLength);

    stream.write(
        reinterpret_cast<const char*>(&networkLength),
        sizeof(networkLength));

    stream.write(
        reinterpret_cast<const char*>(&message),
        sizeof(message));
}

void createReplayFile()
{
    std::ofstream stream(
        ReplayPath,
        std::ios::binary | std::ios::trunc);

    if (!stream.is_open())
        return;

    for (std::size_t index = 0;
         index < SequenceCount;
         ++index)
    {
        const std::uint64_t originalOrderId =
            100000 + index;

        const std::uint64_t replacementOrderId =
            200000 + index;

        AddOrderWireMessage addMessage{};
        addMessage.message_type = 'A';
        addMessage.order_reference_number =
            htobe64(originalOrderId);
        addMessage.buy_sell_indicator = 'B';
        addMessage.shares = htonl(1000);
        addMessage.price =
            htonl(10000 + static_cast<std::uint32_t>(index % 100));

        OrderCancelWireMessage cancelMessage{};
        cancelMessage.message_type = 'X';
        cancelMessage.order_reference_number =
            htobe64(originalOrderId);
        cancelMessage.cancelled_shares = htonl(200);

        OrderExecutedWireMessage executedMessage{};
        executedMessage.message_type = 'E';
        executedMessage.order_reference_number =
            htobe64(originalOrderId);
        executedMessage.executed_shares = htonl(300);
        executedMessage.match_number =
            htobe64(300000 + index);

        OrderReplaceWireMessage replaceMessage{};
        replaceMessage.message_type = 'U';
        replaceMessage.original_order_reference =
            htobe64(originalOrderId);
        replaceMessage.new_order_reference =
            htobe64(replacementOrderId);
        replaceMessage.shares = htonl(400);
        replaceMessage.price =
            htonl(10100 + static_cast<std::uint32_t>(index % 100));

        OrderDeleteWireMessage deleteMessage{};
        deleteMessage.message_type = 'D';
        deleteMessage.order_reference_number =
            htobe64(replacementOrderId);

        writeMessage(stream, addMessage);
        writeMessage(stream, cancelMessage);
        writeMessage(stream, executedMessage);
        writeMessage(stream, replaceMessage);
        writeMessage(stream, deleteMessage);
    }
}

void pipelineReplay(benchmark::State& state)
{
    createReplayFile();

    for (auto _ : state)
    {
        ArrayOrderBook orderBook;
        ITCHHandler handler(orderBook);
        ItchReplayDispatcher dispatcher(handler);
        ItchReplayReader reader(ReplayPath);

        if (!reader.isOpen())
        {
            state.SkipWithError(
                "Failed to open replay benchmark file");
            break;
        }

        std::vector<std::uint8_t> message;
        std::size_t dispatchedMessages = 0;

        while (reader.readNext(message))
        {
            if (!dispatcher.dispatch(
                    message.data(),
                    message.size()))
            {
                state.SkipWithError(
                    "Failed to dispatch replay message");
                break;
            }

            ++dispatchedMessages;
        }

        benchmark::DoNotOptimize(
            orderBook.bestBid());

        benchmark::DoNotOptimize(
            orderBook.bestAsk());

        benchmark::ClobberMemory();

        if (dispatchedMessages !=
            SequenceCount * MessagesPerSequence)
        {
            state.SkipWithError(
                "Unexpected replay message count");
            break;
        }
    }

    state.SetItemsProcessed(
        state.iterations() *
        SequenceCount *
        MessagesPerSequence);

    std::remove(ReplayPath.c_str());
}

BENCHMARK(pipelineReplay);

} // namespace