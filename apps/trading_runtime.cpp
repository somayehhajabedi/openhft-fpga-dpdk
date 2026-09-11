/*
 * Trading Runtime
 * ---------------
 * Composition root for the trading application.
 *
 * Responsibilities:
 * - Parse runtime configuration.
 * - Build and wire the market-data, strategy, risk, gateway,
 *   and OUCH execution components.
 * - Select the execution transport at runtime:
 *     RecordingOuchTransport for local testing/debugging.
 *     TlsOuchTransport for encrypted exchange connectivity.
 * - Run either replay-based or DPDK-based market-data input.
 *
 * This file exists to keep object construction and dependency wiring
 * outside the business-logic components.
 */

#include "dpdk/receiver/itch_udp_payload_sink.hpp"
#include "dpdk/receiver/receiver.hpp"

#include "execution/ouch/ouch_execution_sink.hpp"
#include "execution/ouch/ouch_transport.hpp"
#include "execution/ouch/tls_ouch_transport.hpp"

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
#include "strategy/microstructure_strategy.hpp"
#include "strategy/linear_signal_model.hpp"
#include "execution/ouch/accepted_encoder.hpp"
#include "execution/ouch/ouch_response_dispatcher.hpp"

#include <array>
#include <variant>

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>


namespace
{

enum class InputMode
{
    Replay,
    Dpdk
};


enum class ExecutionMode
{
    Recording,
    Tls
};


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
            << "bytes="
            << length
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


bool parsePort(
    std::string_view value,
    std::uint16_t& port)
{
    unsigned int parsedPort = 0;

    const char* begin =
        value.data();

    const char* end =
        value.data() + value.size();

    const auto [ptr, error] =
        std::from_chars(
            begin,
            end,
            parsedPort);

    if (error != std::errc{} ||
        ptr != end ||
        parsedPort == 0 ||
        parsedPort > 65535)
    {
        return false;
    }

    port =
        static_cast<std::uint16_t>(
            parsedPort);

    return true;
}


void printUsage()
{
    std::cerr
        << "Usage:\n"
        << "  trading_runtime --mode replay <itch_replay_file> "
        << "[--pipeline-cpu <cpu>] "
        << "[--execution recording]\n"
        << '\n'
        << "  trading_runtime --mode replay <itch_replay_file> "
        << "[--pipeline-cpu <cpu>] "
        << "--execution tls "
        << "--host <ip> "
        << "--port <port> "
        << "--ca <certificate>\n"
        << '\n'
        << "  trading_runtime --mode dpdk "
        << "[--rx-cpu <cpu>] "
        << "[--pipeline-cpu <cpu>] "
        << "[--execution recording]\n"
        << '\n'
        << "  trading_runtime --mode dpdk "
        << "[--rx-cpu <cpu>] "
        << "[--pipeline-cpu <cpu>] "
        << "--execution tls "
        << "--host <ip> "
        << "--port <port> "
        << "--ca <certificate>\n";
}

} // namespace


int main(
    int argc,
    char** argv)
{
    if (argc < 3)
    {
        printUsage();
        return 1;
    }

    std::optional<InputMode> mode;

    std::optional<std::string> replayFile;

    std::optional<std::size_t> rxCpu;

    std::optional<std::size_t> pipelineCpu;

    ExecutionMode executionMode =
        ExecutionMode::Recording;

    std::string executionHost =
        "127.0.0.1";

    std::uint16_t executionPort =
        0;

    std::string caCertificatePath;


    for (int index = 1;
         index < argc;
         ++index)
    {
        const std::string_view argument =
            argv[index];

        if (argument == "--mode")
        {
            if (index + 1 >= argc)
            {
                std::cerr
                    << "Missing value for --mode\n";

                printUsage();

                return 1;
            }

            const std::string_view modeValue =
                argv[++index];

            if (modeValue == "replay")
            {
                mode = InputMode::Replay;

                if (index + 1 >= argc)
                {
                    std::cerr
                        << "Missing replay file\n";

                    printUsage();

                    return 1;
                }

                replayFile =
                    argv[++index];
            }
            else if (modeValue == "dpdk")
            {
                mode = InputMode::Dpdk;
            }
            else
            {
                std::cerr
                    << "Unknown mode: "
                    << modeValue
                    << '\n';

                printUsage();

                return 1;
            }

            continue;
        }

        if (argument == "--rx-cpu")
        {
            if (index + 1 >= argc)
            {
                std::cerr
                    << "Missing value for --rx-cpu\n";

                return 1;
            }

            std::size_t cpuIndex = 0;

            if (!parseCpuIndex(
                    argv[index + 1],
                    cpuIndex))
            {
                std::cerr
                    << "Invalid RX CPU index: "
                    << argv[index + 1]
                    << '\n';

                return 1;
            }

            rxCpu =
                cpuIndex;

            ++index;

            continue;
        }

        if (argument == "--pipeline-cpu")
        {
            if (index + 1 >= argc)
            {
                std::cerr
                    << "Missing value for "
                    << "--pipeline-cpu\n";

                return 1;
            }

            std::size_t cpuIndex = 0;

            if (!parseCpuIndex(
                    argv[index + 1],
                    cpuIndex))
            {
                std::cerr
                    << "Invalid pipeline CPU index: "
                    << argv[index + 1]
                    << '\n';

                return 1;
            }

            pipelineCpu =
                cpuIndex;

            ++index;

            continue;
        }

        if (argument == "--execution")
        {
            if (index + 1 >= argc)
            {
                std::cerr
                    << "Missing value for --execution\n";

                return 1;
            }

            const std::string_view executionValue =
                argv[++index];

            if (executionValue == "recording")
            {
                executionMode =
                    ExecutionMode::Recording;
            }
            else if (executionValue == "tls")
            {
                executionMode =
                    ExecutionMode::Tls;
            }
            else
            {
                std::cerr
                    << "Unknown execution mode: "
                    << executionValue
                    << '\n';

                return 1;
            }

            continue;
        }

        if (argument == "--host")
        {
            if (index + 1 >= argc)
            {
                std::cerr
                    << "Missing value for --host\n";

                return 1;
            }

            executionHost =
                argv[++index];

            continue;
        }

        if (argument == "--port")
        {
            if (index + 1 >= argc)
            {
                std::cerr
                    << "Missing value for --port\n";

                return 1;
            }

            if (!parsePort(
                    argv[index + 1],
                    executionPort))
            {
                std::cerr
                    << "Invalid execution port: "
                    << argv[index + 1]
                    << '\n';

                return 1;
            }

            ++index;

            continue;
        }

        if (argument == "--ca")
        {
            if (index + 1 >= argc)
            {
                std::cerr
                    << "Missing value for --ca\n";

                return 1;
            }

            caCertificatePath =
                argv[++index];

            continue;
        }

        std::cerr
            << "Unknown argument: "
            << argument
            << '\n';

        printUsage();

        return 1;
    }


    if (!mode.has_value())
    {
        std::cerr
            << "Input mode was not specified\n";

        printUsage();

        return 1;
    }

    if (mode.value() == InputMode::Replay &&
        !replayFile.has_value())
    {
        std::cerr
            << "Replay mode requires an input file\n";

        return 1;
    }

    if (mode.value() == InputMode::Replay &&
        rxCpu.has_value())
    {
        std::cerr
            << "--rx-cpu is only valid in DPDK mode\n";

        return 1;
    }

    if (executionMode == ExecutionMode::Tls)
    {
        if (executionPort == 0)
        {
            std::cerr
                << "TLS execution requires --port\n";

            return 1;
        }

        if (caCertificatePath.empty())
        {
            std::cerr
                << "TLS execution requires --ca\n";

            return 1;
        }
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

    if (mode.value() == InputMode::Dpdk)
    {
        if (rxCpu.has_value())
        {
            std::cout
                << "RX CPU affinity requested: CPU "
                << rxCpu.value()
                << '\n';
        }
        else
        {
            std::cout
                << "RX CPU affinity: scheduler managed\n";
        }
    }


    //
    // Local reconstructed market-data order book.
    //
    ArrayOrderBook marketBook;

    MarketDataBookConsumer bookConsumer(
        marketBook);


    //
    // Strategy.
    //
    // Uses Level-1 market microstructure features and a lightweight
    // linear signal model to generate directional trading decisions.
    //
    const LinearSignalModel signalModel(
        LinearSignalModel::ModelWeights{
            .imbalance = 0.6,
            .microPrice = 0.4,
            .spread = 0.0
        });

    MicrostructureStrategy strategy(
        marketBook,
        signalModel,
        1001,   // AccountId
        10,     // Order quantity
        0.20,   // Buy score threshold
        -0.20); // Sell score threshold

    StrategyEngine strategyEngine(
        strategy);


    //
    // Execution transport.
    //
    // The concrete implementation is selected here in the
    // composition root. Downstream components depend only on
    // the OuchTransport interface.
    //
    std::unique_ptr<ouch::OuchTransport>
        transport;

    RecordingOuchTransport*
        recordingTransport = nullptr;

    ouch::TlsOuchTransport*
    tlsTransportRaw = nullptr;    


    if (executionMode ==
        ExecutionMode::Recording)
    {
        auto recording =
            std::make_unique<
                RecordingOuchTransport>();

        recordingTransport =
            recording.get();

        transport =
            std::move(recording);

        std::cout
            << "Execution transport: recording\n";
    }
    else
    {
        auto tlsTransport =
            std::make_unique<
                ouch::TlsOuchTransport>(
                    executionHost,
                    executionPort,
                    caCertificatePath);
        tlsTransportRaw = tlsTransport.get();            

        std::cout
            << "Execution transport: TLS\n";

        std::cout
            << "Exchange host: "
            << executionHost
            << '\n';

        std::cout
            << "Exchange port: "
            << executionPort
            << '\n';

        if (!tlsTransport->connect())
        {
            std::cerr
                << "Failed to establish TLS "
                << "connection to exchange\n";

            return 1;
        }

        std::cout
            << "TLS connection established\n";

        transport =
            std::move(tlsTransport);
    }


    //
    // Execution path.
    //
    ouch::OuchExecutionSink executionSink(
        *transport);

    RiskManager riskManager;

    Gateway gateway(
        riskManager,
        executionSink);

    GatewayOrderIntentSink intentSink(
        gateway);


    //
    // Market-data consumer:
    //
    // MarketDataEvent
    //     -> Order Book
    //     -> Strategy
    //     -> Gateway
    //     -> Risk
    //     -> OUCH
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
    // Raw ITCH bytes
    //     -> MarketDataEvent
    //     -> MarketDataPipeline
    //
    ItchReplayDispatcher itchDispatcher(
        pipeline);


    if (mode.value() == InputMode::Replay)
    {
        ItchReplayReader reader(
            replayFile.value());

        if (!reader.isOpen())
        {
            std::cerr
                << "Failed to open replay file: "
                << replayFile.value()
                << '\n';

            return 1;
        }

        pipeline.start();

        std::vector<std::uint8_t> message;

        std::size_t replayedMessages = 0;

        while (reader.readNext(message))
        {
            if (itchDispatcher.dispatch(
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

        if (tlsTransportRaw != nullptr)
{
    std::array<
        std::uint8_t,
        ouch::AcceptedEncoder::AcceptedSize>
        response{};

    if (tlsTransportRaw->receive(
            response.data(),
            response.size()))
    {
        const auto dispatched =
            ouch::OuchResponseDispatcher::dispatch(
                response.data(),
                response.size());

        if (dispatched.has_value() &&
            std::holds_alternative<
                ouch::Accepted>(
                    *dispatched))
        {
            const auto& accepted =
                std::get<
                    ouch::Accepted>(
                        *dispatched);

            std::cout
                << "Received OUCH Accepted. "
                << "userRefNum="
                << accepted.userRefNum
                << '\n';
        }
        else
        {
            std::cerr
                << "Failed to decode OUCH response\n";
        }
    }
    else
    {
        std::cerr
            << "Failed to receive OUCH response\n";
    }
}

        std::cout
            << "Replay complete. "
            << replayedMessages
            << " market-data messages processed.\n";
    }
    else
    {
        //
        // DPDK live input:
        //
        // NIC
        //   -> Ethernet
        //   -> IPv4
        //   -> UDP
        //   -> ItchUdpPayloadSink
        //   -> ItchReplayDispatcher
        //   -> MarketDataPipeline
        //
        ItchUdpPayloadSink udpPayloadSink(
            itchDispatcher);

        Receiver receiver(
            udpPayloadSink,
            rxCpu);

        //
        // For now initialize DPDK with only the executable name.
        // Application/EAL command-line separation will be added later.
        //
        int dpdkArgc = 1;

        char* dpdkArgv[] =
        {
            argv[0],
            nullptr
        };

        if (!receiver.initialize(
                dpdkArgc,
                dpdkArgv))
        {
            std::cerr
                << "Failed to initialize DPDK\n";

            return 1;
        }

        pipeline.start();

        //
        // Current Receiver::run() is intentionally blocking.
        //
        receiver.run();

        pipeline.stop();
    }


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


    if (recordingTransport != nullptr &&
        recordingTransport->sent())
    {
        std::cout
            << "Execution path reached OUCH transport. "
            << "last message bytes="
            << recordingTransport->lastLength()
            << '\n';
    }


    return 0;
}