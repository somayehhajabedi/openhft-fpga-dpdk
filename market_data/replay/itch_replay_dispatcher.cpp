#include "itch_replay_dispatcher.hpp"

#include "gateway/market_data/itch_handler.hpp"

ItchReplayDispatcher::ItchReplayDispatcher(
    ITCHHandler& handler)
    : handler_(handler)
{
}

bool ItchReplayDispatcher::dispatch(
    const std::uint8_t* message,
    std::size_t length)
{
    if (message == nullptr || length == 0)
        return false;

    const char messageType =
        static_cast<char>(message[0]);

    switch (messageType)
    {
        case 'A':
            return handler_.onAddOrder(message, length);

        case 'X':
            return handler_.onOrderCancel(message, length);

        case 'D':
            return handler_.onOrderDelete(message, length);

        case 'E':
            return handler_.onOrderExecuted(message, length);

        case 'U':
            return handler_.onOrderReplace(message, length);

        default:
            return false;
    }
}