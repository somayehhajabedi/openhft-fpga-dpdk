
#pragma once

#include "execution/ouch/ouch_messages.hpp"
#include "execution/ouch/ouch_transport.hpp"
#include "gateway/order_execution_sink.hpp"


#include <cstdint>

namespace ouch
{

class OuchExecutionSink final
    : public OrderExecutionSink
{
public:
    explicit OuchExecutionSink(
        OuchTransport& transport);

    bool submit(
        const OrderIntent& intent) override;

private:
    [[nodiscard]]
    UserRefNum nextUserRefNum() noexcept;

    OuchTransport& transport_;

    UserRefNum nextUserRefNum_{1};
};

} // namespace ouch
