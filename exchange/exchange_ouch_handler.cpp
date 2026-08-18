#include "exchange/exchange_ouch_handler.hpp"

#include "execution/ouch/ouch_decoder.hpp"
#include "orderbook/software/matching_engine.hpp"

ExchangeOuchHandler::ExchangeOuchHandler(
    MatchingEngine& matchingEngine)
    :
    matchingEngine_(matchingEngine)
{
}

bool ExchangeOuchHandler::handleEnterOrder(
    AccountId accountId,
    const std::uint8_t* data,
    std::size_t length)
{
    const auto enterOrder =
        ouch::OuchDecoder::decodeEnterOrder(
            data,
            length);

    if (!enterOrder.has_value())
    {
        return false;
    }

    return matchingEngine_.submitOrder(
        accountId,
        enterOrder->side,
        static_cast<Price>(
            enterOrder->price),
        enterOrder->quantity);
}

