#include "exchange/exchange_ouch_handler.hpp"

#include "execution/ouch/accepted_encoder.hpp"
#include "execution/ouch/ouch_decoder.hpp"
#include "orderbook/software/matching_engine.hpp"

ExchangeOuchHandler::ExchangeOuchHandler(
    MatchingEngine& matchingEngine,
    ExchangeOrderSessionMap& sessionMap)
    :
    matchingEngine_(matchingEngine),
    sessionMap_(sessionMap)
{
}

std::optional<ExchangeOuchHandler::ResponseBuffer>
ExchangeOuchHandler::handleEnterOrder(
    AccountId accountId,
    const std::uint8_t* data,
    std::size_t length)
{
    //
    // Decode the incoming OUCH EnterOrder message.
    //
    const auto enterOrder =
        ouch::OuchDecoder::decodeEnterOrder(
            data,
            length);

    if (!enterOrder.has_value())
    {
        return std::nullopt;
    }

    //
    // Prepare the internal order first.
    //
    // IMPORTANT:
    // prepareOrder() assigns the Exchange OrderId
    // but does NOT send the order to the matching
    // engine yet.
    //
    Order* order =
        matchingEngine_.prepareOrder(
            accountId,
            enterOrder->side,
            static_cast<Price>(
                enterOrder->price),
            enterOrder->quantity);

    if (order == nullptr)
    {
        return std::nullopt;
    }

    const OrderId orderId =
        order->id;

    //
    // Register the relationship BEFORE matching.
    //
    // This is important because process() may
    // immediately generate a Trade, which causes
    // ExchangeExecutionListener::onTrade() to run.
    //
    sessionMap_.registerOrder(
        orderId,
        enterOrder->userRefNum);

    //
    // Now the order can safely enter the
    // MatchingEngine.
    //
    matchingEngine_.process(
        order);

    //
    // Build the OUCH Accepted response.
    //
    ouch::Accepted accepted{};

    accepted.timestamp = 0;

    accepted.userRefNum =
        enterOrder->userRefNum;

    accepted.side =
        enterOrder->side;

    accepted.quantity =
        enterOrder->quantity;

    accepted.symbol =
        enterOrder->symbol;

    accepted.price =
        enterOrder->price;

    accepted.timeInForce =
        enterOrder->timeInForce;

    accepted.display =
        enterOrder->display;

    accepted.orderReferenceNumber =
        orderId;

    accepted.capacity =
        enterOrder->capacity;

    accepted.isoEligibility =
        enterOrder->isoEligibility;

    accepted.crossType =
        enterOrder->crossType;

    accepted.orderState =
        ouch::OrderState::Live;

    accepted.clOrdId =
        enterOrder->clOrdId;

    accepted.appendageLength = 0;

    //
    // Encode Accepted into the OUCH wire format.
    //
    return ouch::AcceptedEncoder::encode(
        accepted);
}
