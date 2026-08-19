#pragma once

#include "events/i_trade_listener.hpp"
#include "exchange/exchange_order_session_map.hpp"
#include "execution/ouch/executed_encoder.hpp"

#include <array>
#include <cstddef>
#include <optional>

class ExchangeExecutionListener final
    : public ITradeListener
{
public:
    struct ExecutionResponses
    {
        std::optional<ouch::ExecutedEncoder::Buffer>
            buyResponse;

        std::optional<ouch::ExecutedEncoder::Buffer>
            sellResponse;
    };

    explicit ExchangeExecutionListener(
        ExchangeOrderSessionMap& sessionMap);

    void onTrade(
        const Trade& trade) override;

    [[nodiscard]]
    const ExecutionResponses& lastResponses() const noexcept;

private:
    ExchangeOrderSessionMap& sessionMap_;
    ExecutionResponses lastResponses_{};
};
