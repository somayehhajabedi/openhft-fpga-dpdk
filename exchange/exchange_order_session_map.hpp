#pragma once

#include "common/types.hpp"
#include "execution/ouch/ouch_messages.hpp"

#include <optional>
#include <unordered_map>

class ExchangeOrderSessionMap
{
public:
    void registerOrder(
        OrderId orderId,
        ouch::UserRefNum userRefNum)
    {
        orderToUserRef_[orderId] =
            userRefNum;
    }

    [[nodiscard]]
    std::optional<ouch::UserRefNum> find(
        OrderId orderId) const
    {
        const auto it =
            orderToUserRef_.find(orderId);

        if (it == orderToUserRef_.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    void erase(
        OrderId orderId)
    {
        orderToUserRef_.erase(orderId);
    }

private:
    std::unordered_map<
        OrderId,
        ouch::UserRefNum> orderToUserRef_;
};
