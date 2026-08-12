#include "itch_replay_dispatcher.hpp"

#include "dpdk/parser/itch/messages/add_order.hpp"
#include "dpdk/parser/itch/messages/order_cancel_parser.hpp"
#include "dpdk/parser/itch/messages/order_delete_parser.hpp"
#include "dpdk/parser/itch/messages/order_executed_parser.hpp"
#include "dpdk/parser/itch/messages/order_replace_parser.hpp"

#include "dpdk/parser/itch/mapper/add_order_mapper.hpp"
#include "dpdk/parser/itch/mapper/order_cancel_mapper.hpp"
#include "dpdk/parser/itch/mapper/order_delete_mapper.hpp"
#include "dpdk/parser/itch/mapper/order_executed_mapper.hpp"
#include "dpdk/parser/itch/mapper/order_replace_mapper.hpp"

#include "pipeline/market_data_event.hpp"

ItchReplayDispatcher::ItchReplayDispatcher(
    MarketDataEventSink& sink)
    :
    sink_(sink)
{
}

bool ItchReplayDispatcher::dispatch(
    const std::uint8_t* message,
    std::size_t length)
{
    if (message == nullptr || length == 0)
    {
        return false;
    }

    const char messageType =
        static_cast<char>(message[0]);

    switch (messageType)
    {
        case 'A':
        {
            const AddOrderWireMessage* wire =
                AddOrderParser::parse(
                    message,
                    length);

            if (wire == nullptr)
            {
                return false;
            }

            const AddOrder order =
                AddOrderMapper::fromWire(wire);

            const MarketDataEvent event{
                .type = MarketDataEventType::AddOrder,
                .orderId = order.orderReferenceNumber,
                .newOrderId = 0,
                .accountId = 0,
                .side =
                    order.isBuy
                        ? Side::Buy
                        : Side::Sell,
                .price =
                    static_cast<Price>(
                        order.price),
                .quantity =
                    static_cast<Quantity>(
                        order.shares)
            };

	    return sink_.submit(event);

        }

        case 'X':
        {
            const OrderCancelWireMessage* wire =
                OrderCancelParser::parse(
                    message,
                    length);

            if (wire == nullptr)
            {
                return false;
            }

            const OrderCancel cancel =
                OrderCancelMapper::fromWire(wire);

            const MarketDataEvent event{
                .type = MarketDataEventType::CancelOrder,
                .orderId =
                    cancel.orderReferenceNumber,
                .quantity =
                    static_cast<Quantity>(
                        cancel.cancelledShares)
            };
	    return sink_.submit(event);
        }

        case 'D':
        {
            const OrderDeleteWireMessage* wire =
                OrderDeleteParser::parse(
                    message,
                    length);

            if (wire == nullptr)
            {
                return false;
            }

            const OrderDelete deletion =
                OrderDeleteMapper::fromWire(wire);

            const MarketDataEvent event{
                .type = MarketDataEventType::DeleteOrder,
                .orderId =
                    deletion.orderReferenceNumber
            };
	
	    return sink_.submit(event);
        }

        case 'E':
        {
            const OrderExecutedWireMessage* wire =
                OrderExecutedParser::parse(
                    message,
                    length);

            if (wire == nullptr)
            {
                return false;
            }

            const OrderExecuted execution =
                OrderExecutedMapper::fromWire(wire);

            const MarketDataEvent event{
                .type = MarketDataEventType::ExecuteOrder,
                .orderId =
                    execution.orderReferenceNumber,
                .quantity =
                    static_cast<Quantity>(
                        execution.executedShares)
            };
	    
	    return sink_.submit(event);
        }

        case 'U':
        {
            const OrderReplaceWireMessage* wire =
                OrderReplaceParser::parse(
                    message,
                    length);

            if (wire == nullptr)
            {
                return false;
            }

            const OrderReplace replacement =
                OrderReplaceMapper::fromWire(wire);

            const MarketDataEvent event{
                .type = MarketDataEventType::ReplaceOrder,
                .orderId =
                    replacement.originalOrderReferenceNumber,
                .newOrderId =
                    replacement.newOrderReferenceNumber,
                .price =
                    static_cast<Price>(
                        replacement.price),
                .quantity =
                    static_cast<Quantity>(
                        replacement.shares)
            };

	    return sink_.submit(event);
        }

        default:
        {
            return false;
        }
    }
}
