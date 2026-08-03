#include <benchmark/benchmark.h>

#include "array_order_book.hpp"
#include "order.hpp"

static Order makeOrder(
    OrderId id,
    Side side,
    Price price,
    Quantity quantity = 100)
{
    return Order{
        .id = id,
        .account_id = 100,
        .side = side,
        .price = price,
        .quantity = quantity
    };
}

static void BM_RefreshBestBidLargeGap(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        ArrayOrderBook book;

        Order bestBid =
            makeOrder(1, Side::Buy, 90000);

        Order nextBid =
            makeOrder(2, Side::Buy, 1000);

        book.addOrder(&bestBid);
        book.addOrder(&nextBid);

        state.ResumeTiming();

        bool result =
            book.cancelOrder(bestBid.id);

        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(book.bestBid());
    }
}

static void BM_RefreshBestAskLargeGap(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        ArrayOrderBook book;

        Order bestAsk =
            makeOrder(1, Side::Sell, 1000);

        Order nextAsk =
            makeOrder(2, Side::Sell, 90000);

        book.addOrder(&bestAsk);
        book.addOrder(&nextAsk);

        state.ResumeTiming();

        bool result =
            book.cancelOrder(bestAsk.id);

        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(book.bestAsk());
    }
}

static void BM_RefreshBestBidSameWord(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        ArrayOrderBook book;

        Order bestBid =
            makeOrder(1, Side::Buy, 190);

        Order nextBid =
            makeOrder(2, Side::Buy, 189);

        book.addOrder(&bestBid);
        book.addOrder(&nextBid);

        state.ResumeTiming();

        bool result =
            book.cancelOrder(bestBid.id);

        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(book.bestBid());
    }
}

static void BM_RefreshBestAskSameWord(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        ArrayOrderBook book;

        Order bestAsk =
            makeOrder(1, Side::Sell, 100);

        Order nextAsk =
            makeOrder(2, Side::Sell, 101);

        book.addOrder(&bestAsk);
        book.addOrder(&nextAsk);

        state.ResumeTiming();

        bool result =
            book.cancelOrder(bestAsk.id);

        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(book.bestAsk());
    }
}

static void BM_ProfileRefreshBestBidLargeGap(
    benchmark::State& state)
{
    ArrayOrderBook book;

    Order bestOrder = makeOrder(
        1,
        Side::Buy,
        90000);

    Order fallbackOrder = makeOrder(
        2,
        Side::Buy,
        1000);

    book.addOrder(&bestOrder);
    book.addOrder(&fallbackOrder);

    for (auto _ : state)
    {
        const bool cancelled =
            book.cancelOrder(bestOrder.id);

        benchmark::DoNotOptimize(cancelled);

        // Restore the initial state for the next iteration.
        book.addOrder(&bestOrder);
    }
}

BENCHMARK(BM_ProfileRefreshBestBidLargeGap)
    ->Iterations(1000000);

BENCHMARK(BM_RefreshBestBidLargeGap)
    ->Iterations(1000)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_RefreshBestAskLargeGap)
    ->Iterations(1000)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_RefreshBestBidSameWord)
    ->Iterations(1000)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_RefreshBestAskSameWord)
    ->Iterations(1000)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_ProfileRefreshBestBidLargeGap)
    ->Iterations(1000000);