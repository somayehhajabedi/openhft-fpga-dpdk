#pragma once

#include <memory>
#include <string>

namespace spdlog
{
class logger;
}

class AsyncLogger
{
public:
    static void initialize(
        const std::string& filePath);

    static void shutdown();

    static std::shared_ptr<spdlog::logger>
    get();

private:
    AsyncLogger() = delete;
};
