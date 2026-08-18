#pragma once

#include "common/types.hpp"

#include <cstddef>
#include <cstdint>

class MatchingEngine;

class ExchangeOuchHandler
{
public:
    explicit ExchangeOuchHandler(
        MatchingEngine& matchingEngine);

    [[nodiscard]]
    bool handleEnterOrder(
        AccountId accountId,
        const std::uint8_t* data,
        std::size_t length);

private:
    MatchingEngine& matchingEngine_;
};
