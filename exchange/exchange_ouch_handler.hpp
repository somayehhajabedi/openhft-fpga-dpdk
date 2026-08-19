#pragma once

#include "common/types.hpp"
#include "exchange/exchange_order_session_map.hpp"
#include "execution/ouch/accepted_encoder.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

class MatchingEngine;

class ExchangeOuchHandler
{
public:
    using ResponseBuffer =
        ouch::AcceptedEncoder::Buffer;

    ExchangeOuchHandler(
        MatchingEngine& matchingEngine,
        ExchangeOrderSessionMap& sessionMap);

    [[nodiscard]]
    std::optional<ResponseBuffer> handleEnterOrder(
        AccountId accountId,
        const std::uint8_t* data,
        std::size_t length);

private:
    MatchingEngine& matchingEngine_;
    ExchangeOrderSessionMap& sessionMap_;
};
