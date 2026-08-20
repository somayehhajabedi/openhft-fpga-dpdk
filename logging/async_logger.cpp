#include "logging/async_logger.hpp"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace
{

std::shared_ptr<spdlog::logger> logger;

} // namespace

void AsyncLogger::initialize(
    const std::string& filePath)
{
    if (logger)
    {
        return;
    }

    spdlog::init_thread_pool(
        8192,
        1);

    auto sink =
        std::make_shared<
            spdlog::sinks::basic_file_sink_mt>(
                filePath,
                true);

    logger =
        std::make_shared<
            spdlog::async_logger>(
                "openhft",
                sink,
                spdlog::thread_pool(),
                spdlog::async_overflow_policy::overrun_oldest);

    logger->set_level(
        spdlog::level::info);

    logger->set_pattern(
        "%Y-%m-%d %H:%M:%S.%e [%l] %v");

    spdlog::register_logger(
        logger);
}

void AsyncLogger::shutdown()
{
    if (!logger)
    {
        return;
    }

    logger->flush();

    spdlog::drop(
        "openhft");

    logger.reset();

    spdlog::shutdown();
}

std::shared_ptr<spdlog::logger>
AsyncLogger::get()
{
    return logger;
}
