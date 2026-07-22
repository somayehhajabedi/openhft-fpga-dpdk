#pragma once

#include "../events/i_trade_listener.hpp"

#include <unordered_map>

class PositionManager : public ITradeListener
{
public:
    void onTrade(const Trade& trade) override;

    int position(AccountId account_id) const;

private:
    std::unordered_map<AccountId, int> positions_;
};