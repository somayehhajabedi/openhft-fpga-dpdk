#include <gtest/gtest.h>

#include "logging/async_logger.hpp"

#include <spdlog/spdlog.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdio>
#include <iterator>

TEST(
    AsyncLoggerTest,
    WritesMessageToFile)
{
    const std::string logPath =
        "async_logger_test.log";

    std::remove(
        logPath.c_str());

    AsyncLogger::initialize(
        logPath);

    auto logger =
        AsyncLogger::get();

    ASSERT_NE(
        logger,
        nullptr);

    logger->info(
        "test-message value={}",
        42);

    logger->flush();

    //
    // Give the async worker a short moment
    // to process the queued log message.
    //
    std::this_thread::sleep_for(
        std::chrono::milliseconds(50));

    AsyncLogger::shutdown();

    std::ifstream stream(
        logPath);

    ASSERT_TRUE(
        stream.is_open());

    std::string contents(
        (std::istreambuf_iterator<char>(
            stream)),
        std::istreambuf_iterator<char>());

    EXPECT_NE(
        contents.find(
            "test-message value=42"),
        std::string::npos);

    stream.close();

    std::remove(
        logPath.c_str());
}
