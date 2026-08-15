#include "execution/ouch/ouch_execution_sink.hpp"

#include "execution/ouch/ouch_encoder.hpp"

namespace ouch
{

OuchExecutionSink::OuchExecutionSink(
    OuchTransport& transport)
    :
    transport_(transport)
{
}

bool OuchExecutionSink::submit(
    const OrderIntent& intent)
{
    EnterOrder order{};

    order.userRefNum =
        nextUserRefNum();

    order.side =
        intent.side;

    order.quantity =
        intent.quantity;

    order.symbol =
        intent.symbol;

    order.price =
        static_cast<std::uint64_t>(
            intent.price);

    order.timeInForce =
        TimeInForce::Day;

    order.display =
        Display::Visible;

    order.capacity =
        Capacity::Agency;

    order.isoEligibility =
        IsoEligibility::NotEligible;

    order.crossType =
        CrossType::ContinuousMarket;

    order.clOrdId = {};

    order.appendageLength = 0;

    const auto buffer =
        OuchEncoder::encode(order);

    return transport_.send(
        buffer.data(),
        buffer.size());
}

UserRefNum OuchExecutionSink::nextUserRefNum() noexcept
{
    return nextUserRefNum_++;
}

} // namespace ouch
