#include "position_manager.hpp"

void PositionManager::onTrade(const Trade& trade)
{
    positions_[trade.buy_account_id] += trade.quantity;
    positions_[trade.sell_account_id] -= trade.quantity;
}

int PositionManager::position(AccountId account_id) const
{
    auto it = positions_.find(account_id);

    if (it == positions_.end())
        return 0;

    return it->second;
}