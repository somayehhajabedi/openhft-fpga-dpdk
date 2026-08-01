#include <chrono>
#include <cstddef>
#include <iostream>
#include <vector>

#include <unistd.h>
#include <cstdlib>

#include <optional>
#include <string_view>

#include "dispatcher/event_dispatcher.hpp"
#include "orderbook/software/matching_engine.hpp"
#include "orderbook/software/order.hpp"
#include "common/thread_affinity.hpp"

namespace
{

constexpr std::size_t BatchSize = 3'000;
constexpr std::size_t BatchCount = 10'000;
constexpr std::size_t WarmupBatchCount = 20;

constexpr Quantity OrderQuantity = 100;
constexpr Price LowestPrice = 100;
constexpr std::size_t PriceLevelCount = 16;

struct OrderPair
{
    Order buy;
    Order sell;
};

void resetOrder(Order& order)
{
    order.quantity = OrderQuantity;
    order.level = nullptr;
    order.prev = nullptr;
    order.next = nullptr;
}

void runBatch(
    MatchingEngine& engine,
    std::vector<OrderPair>& order_pairs)
{
    // Reset and insert all buy orders first.
    // The buy orders are distributed across multiple price levels.
    for (OrderPair& pair : order_pairs)
    {
        resetOrder(pair.buy);
        resetOrder(pair.sell);

        engine.process(&pair.buy);
    }

    // Every sell order uses the lowest price.
    // Therefore, it can cross any resting buy order from 100 to 115.
    //
    // Because the number of sell orders equals the number of buy orders,
    // all resting buy orders are fully matched and removed by the end
    // of every batch.
    for (OrderPair& pair : order_pairs)
    {
        engine.process(&pair.sell);
    }
}

} // namespace

namespace
{

std::optional<std::size_t> parseCpuArgument(
    int argc,
    char* argv[])
{
    if (argc == 1)
    {
        return std::nullopt;
    }

    if (argc != 3 ||
        std::string_view(argv[1]) != "--cpu")
    {
        std::cerr
            << "Usage: "
            << argv[0]
            << " [--cpu CPU_INDEX]\n";

        std::exit(EXIT_FAILURE);
    }

    char* end = nullptr;

    const unsigned long cpu =
        std::strtoul(argv[2], &end, 10);

    if (end == argv[2] || *end != '\0')
    {
        std::cerr << "Invalid CPU index: "
                  << argv[2] << '\n';

        std::exit(EXIT_FAILURE);
    }

    return static_cast<std::size_t>(cpu);
}

}


    int main(int argc, char* argv[])
{
    const auto cpu = parseCpuArgument(argc, argv);

    if (cpu.has_value())
    {
        if (!pinCurrentThreadToCpu(*cpu))
        {
            std::cerr
                << "Failed to pin profiling thread to CPU "
                << *cpu
                << '\n';

            return EXIT_FAILURE;
        }

        std::cout
            << "CPU affinity enabled: CPU "
            << *cpu
            << '\n';
    }
    else
    {
        std::cout << "CPU affinity disabled\n";
    }


    std::vector<OrderPair> order_pairs;
    order_pairs.reserve(BatchSize);

    for (std::size_t i = 0; i < BatchSize; ++i)
    {
        const OrderId buy_id =
            static_cast<OrderId>((i * 2) + 1);

        const OrderId sell_id =
            static_cast<OrderId>((i * 2) + 2);

        const Price buy_price =
            static_cast<Price>(
                LowestPrice + (i % PriceLevelCount));

        order_pairs.push_back(OrderPair{
            Order{
                buy_id,
                static_cast<AccountId>(1001),
                Side::Buy,
                buy_price,
                OrderQuantity
            },
            Order{
                sell_id,
                static_cast<AccountId>(2001),
                Side::Sell,
                LowestPrice,
                OrderQuantity
            }
        });
    }

    EventDispatcher dispatcher;
    MatchingEngine engine(dispatcher);

    // Warm up instruction cache, data cache, and branch predictor.
    for (std::size_t batch = 0;
         batch < WarmupBatchCount;
         ++batch)
    {
        runBatch(engine, order_pairs);
    }

    std::cout
        << "Profiling workload is ready.\n"
        << "PID: " << getpid() << '\n'
        << "Pairs per batch: " << BatchSize << '\n'
        << "Measured batches: " << BatchCount << '\n'
        << "Buy price levels: " << PriceLevelCount << '\n'
        << "Sell price: " << LowestPrice << '\n'
        << "Press Enter after attaching perf.\n";

    std::cin.get();

    const auto start =
        std::chrono::steady_clock::now();

    for (std::size_t batch = 0;
         batch < BatchCount;
         ++batch)
    {
        runBatch(engine, order_pairs);
    }

    const auto end =
        std::chrono::steady_clock::now();

    constexpr std::size_t OperationsPerPair = 2;

    const std::size_t total_orders =
        BatchSize *
        BatchCount *
        OperationsPerPair;

    const double elapsed_seconds =
        std::chrono::duration<double>(
            end - start).count();

    const double throughput =
        static_cast<double>(total_orders) /
        elapsed_seconds;

    const double nanoseconds_per_order =
        elapsed_seconds *
        1'000'000'000.0 /
        static_cast<double>(total_orders);

    std::cout
        << "Orders processed: "
        << total_orders << '\n'
        << "Elapsed: "
        << elapsed_seconds
        << " seconds\n"
        << "Throughput: "
        << throughput
        << " orders/sec\n"
        << "Average latency: "
        << nanoseconds_per_order
        << " ns/order\n";

    return 0;
}