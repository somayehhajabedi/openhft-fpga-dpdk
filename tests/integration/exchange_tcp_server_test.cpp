#include <gtest/gtest.h>

#include "dispatcher/event_dispatcher.hpp"
#include "execution/ouch/tls_ouch_transport.hpp"

#include "exchange/exchange_order_session_map.hpp"
#include "exchange/exchange_ouch_handler.hpp"
#include "exchange/exchange_tcp_server.hpp"

#include "execution/ouch/accepted_encoder.hpp"
#include "execution/ouch/ouch_encoder.hpp"
#include "execution/ouch/ouch_messages.hpp"
#include "execution/ouch/ouch_response_dispatcher.hpp"
#include "execution/ouch/tcp_ouch_transport.hpp"

#include "orderbook/software/matching_engine.hpp"

#include "position/position_manager.hpp"

#include <spdlog/spdlog.h>

#include "logging/async_logger.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <array>
#include <atomic>
#include <cstdint>
#include <thread>
#include <variant>


TEST(
    ExchangeTcpServerTest,
    EnterOrderTravelsOverTcpAndReturnsAccepted)
{
    //
    // Exchange-side components.
    //
    EventDispatcher dispatcher;

    MatchingEngine matchingEngine(
        dispatcher);

    ExchangeOrderSessionMap sessionMap;

    ExchangeOuchHandler handler(
        matchingEngine,
        sessionMap);

    //
    // Port 0 means:
    // let the kernel choose a free loopback port.
    //
    ExchangeTcpServer server(
        0,
        1001,
        handler);

    ASSERT_TRUE(
        server.start());

    ASSERT_NE(
        server.port(),
        0U);

    //
    // Run the epoll event loop on the Exchange side.
    //
    std::atomic<bool> running{
        true};

    std::atomic<bool> serverOk{
        true};

    std::thread serverThread(
        [&]()
        {
            while (running.load())
            {
                if (!server.pollOnce(10))
                {
                    serverOk.store(false);
                    break;
                }
            }
        });

    //
    // Client-side TCP connection.
    //
    ouch::TcpOuchTransport transport(
        "127.0.0.1",
        server.port());

    ASSERT_TRUE(
        transport.connect());

    //
    // Client creates an OUCH EnterOrder.
    //
    ouch::EnterOrder order{};

    order.userRefNum = 555;

    order.side =
        Side::Buy;

    order.quantity =
        10;

    order.symbol = {
        'A', 'A', 'P', 'L',
        ' ', ' ', ' ', ' '
    };

    order.price =
        99;

    //
    // EnterOrder object
    //       ↓
    // 47-byte OUCH message
    //
    const auto request =
        ouch::OuchEncoder::encode(
            order);

    ASSERT_TRUE(
        transport.send(
            request.data(),
            request.size()));

    //
    // ExchangeTcpServer should:
    //
    // epoll
    //   ↓
    // recv 47 bytes
    //   ↓
    // ExchangeOuchHandler
    //   ↓
    // MatchingEngine
    //   ↓
    // Accepted
    //   ↓
    // send 64 bytes
    //
    std::array<
        std::uint8_t,
        ouch::AcceptedEncoder::AcceptedSize>
        response{};

    ASSERT_TRUE(
        transport.receive(
            response.data(),
            response.size()));

    //
    // Client processes the returned OUCH response
    // through the real response dispatcher.
    //
    const auto dispatched =
        ouch::OuchResponseDispatcher::dispatch(
            response.data(),
            response.size());

    ASSERT_TRUE(
        dispatched.has_value());

    ASSERT_TRUE(
        std::holds_alternative<ouch::Accepted>(
            *dispatched));

    const auto& accepted =
        std::get<ouch::Accepted>(
            *dispatched);

    EXPECT_EQ(
        accepted.userRefNum,
        order.userRefNum);

    EXPECT_EQ(
        accepted.side,
        Side::Buy);

    EXPECT_EQ(
        accepted.quantity,
        10U);

    EXPECT_EQ(
        accepted.symbol,
        order.symbol);

    EXPECT_EQ(
        accepted.price,
        99U);

    EXPECT_NE(
        accepted.orderReferenceNumber,
        0U);

    //
    // Shut down client/server.
    //
    transport.close();

    running.store(false);

    serverThread.join();

    EXPECT_TRUE(
        serverOk.load());

    server.stop();
}


TEST(
    ExchangeTcpServerTest,
    EnterOrderTravelsOverTlsAndReturnsAccepted)
{
    EventDispatcher dispatcher;

    MatchingEngine matchingEngine(
        dispatcher);

    ExchangeOrderSessionMap sessionMap;

    ExchangeOuchHandler handler(
        matchingEngine,
        sessionMap);

    const std::string certificatePath =
    std::string(TEST_PROJECT_ROOT) +
    "/certs/server.crt";

    const std::string privateKeyPath =
        std::string(TEST_PROJECT_ROOT) +
        "/certs/server.key";

    ExchangeTcpServer server(
        0,
        1001,
        handler,
        certificatePath,
        privateKeyPath);


    ASSERT_TRUE(
        server.start());

    ASSERT_NE(
        server.port(),
        0U);

    std::atomic<bool> running{
        true};

    std::atomic<bool> serverOk{
        true};

    std::thread serverThread(
        [&]()
        {
            while (running.load())
            {
                if (!server.pollOnce(10))
                {
                    serverOk.store(false);
                    break;
                }
            }
        });

    ouch::TlsOuchTransport transport(
    "127.0.0.1",
    server.port(),
    certificatePath);

    ASSERT_TRUE(
        transport.connect());

    ouch::EnterOrder order{};

    order.userRefNum = 777;

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

    const auto request =
        ouch::OuchEncoder::encode(
            order);

    ASSERT_TRUE(
        transport.send(
            request.data(),
            request.size()));

    std::array<
        std::uint8_t,
        ouch::AcceptedEncoder::AcceptedSize>
        response{};

    ASSERT_TRUE(
        transport.receive(
            response.data(),
            response.size()));

    const auto dispatched =
        ouch::OuchResponseDispatcher::dispatch(
            response.data(),
            response.size());

    ASSERT_TRUE(
        dispatched.has_value());

    ASSERT_TRUE(
        std::holds_alternative<ouch::Accepted>(
            *dispatched));

    const auto& accepted =
        std::get<ouch::Accepted>(
            *dispatched);

    EXPECT_EQ(
        accepted.userRefNum,
        order.userRefNum);

    EXPECT_EQ(
        accepted.side,
        Side::Buy);

    EXPECT_EQ(
        accepted.quantity,
        25U);

    EXPECT_EQ(
        accepted.symbol,
        order.symbol);

    EXPECT_EQ(
        accepted.price,
        101U);

    EXPECT_NE(
        accepted.orderReferenceNumber,
        0U);

    transport.close();

    running.store(false);

    serverThread.join();

    EXPECT_TRUE(
        serverOk.load());

    server.stop();
}



TEST(
    ExchangeTcpServerTest,
    WritesServerLifecycleEventsToAsyncLog)
{
    const std::string logPath =
        "exchange_tcp_server_test.log";

    std::remove(
        logPath.c_str());

    AsyncLogger::initialize(
        logPath);

    PositionManager positionManager;

    EventDispatcher dispatcher;

    dispatcher.addListener(
        &positionManager);

    MatchingEngine engine(
        dispatcher);

    ExchangeOrderSessionMap sessionMap;

    ExchangeOuchHandler handler(
        engine,
        sessionMap);

    ExchangeTcpServer server(
        0,
        1001,
        handler);

    ASSERT_TRUE(
        server.start());

    const std::uint16_t port =
        server.port();

    std::thread serverThread(
        [&server]()
        {
            for (int i = 0;
                 i < 20;
                 ++i)
            {
                if (!server.pollOnce(50))
                {
                    break;
                }
            }
        });

    ouch::TcpOuchTransport transport(
        "127.0.0.1",
        port);

    ASSERT_TRUE(
        transport.connect());

    //
    // Let epoll observe the connection.
    //
    std::this_thread::sleep_for(
        std::chrono::milliseconds(50));

    transport.close();

    //
    // Let epoll observe the peer close.
    //
    std::this_thread::sleep_for(
        std::chrono::milliseconds(100));

    serverThread.join();

    server.stop();

    auto logger =
        AsyncLogger::get();

    ASSERT_NE(
        logger,
        nullptr);

    logger->flush();

    //
    // Async logger: allow the worker to drain
    // before shutting it down.
    //
    std::this_thread::sleep_for(
        std::chrono::milliseconds(50));

    AsyncLogger::shutdown();

    std::ifstream stream(
        logPath);

    ASSERT_TRUE(
        stream.is_open());

    const std::string contents(
        (std::istreambuf_iterator<char>(
            stream)),
        std::istreambuf_iterator<char>());

    EXPECT_NE(
        contents.find(
            "Exchange TCP server started"),
        std::string::npos);

    EXPECT_NE(
        contents.find(
            "TCP client connected"),
        std::string::npos);

    EXPECT_NE(
        contents.find(
            "TCP client disconnected"),
        std::string::npos);

    stream.close();

    std::remove(
        logPath.c_str());
}
