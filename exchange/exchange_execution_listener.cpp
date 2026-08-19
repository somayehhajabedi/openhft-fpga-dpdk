#include "exchange/exchange_execution_listener.hpp"

#include "execution/ouch/ouch_messages.hpp"

ExchangeExecutionListener::ExchangeExecutionListener(
    ExchangeOrderSessionMap& sessionMap)
    :
    sessionMap_(sessionMap)
{
}

void ExchangeExecutionListener::onTrade(
    const Trade& trade)
{
    lastResponses_ = {};

    const auto buyUserRef =
        sessionMap_.find(
            trade.buy_order_id);

    if (buyUserRef.has_value())
    {
        ouch::Executed executed{};

        executed.timestamp = 0;
        executed.userRefNum = *buyUserRef;
        executed.quantity = trade.quantity;
        executed.price =
            static_cast<std::uint64_t>(
                trade.price);
        executed.liquidityFlag = 'R';
        executed.matchNumber = trade.sequence;
        executed.appendageLength = 0;

        lastResponses_.buyResponse =
            ouch::ExecutedEncoder::encode(
                executed);
    }

    const auto sellUserRef =
        sessionMap_.find(
            trade.sell_order_id);

    if (sellUserRef.has_value())
    {
        ouch::Executed executed{};

        executed.timestamp = 0;
        executed.userRefNum = *sellUserRef;
        executed.quantity = trade.quantity;
        executed.price =
            static_cast<std::uint64_t>(
                trade.price);
        executed.liquidityFlag = 'R';
        executed.matchNumber = trade.sequence;
        executed.appendageLength = 0;

        lastResponses_.sellResponse =
            ouch::ExecutedEncoder::encode(
                executed);
    }
}

const ExchangeExecutionListener::ExecutionResponses&
ExchangeExecutionListener::lastResponses() const noexcept
{
    return lastResponses_;
}
