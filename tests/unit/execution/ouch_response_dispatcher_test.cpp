#include <gtest/gtest.h>

#include "execution/ouch/ouch_response_dispatcher.hpp"

#include <array>
#include <variant>

TEST(
    OuchResponseDispatcherTest,
    DispatchesExecutedMessage)
{
    std::array<std::uint8_t, 36> data{};

    data[0] = 'E';

    // UserRefNum = 1
    data[12] = 0x01;

    // Executed quantity = 25
    data[16] = 0x19;

    // Execution price = 12345
    data[23] = 0x30;
    data[24] = 0x39;

    // Liquidity flag
    data[25] = 'A';

    // Match number = 99
    data[33] = 0x63;

    // Appendage length = 0
    data[34] = 0x00;
    data[35] = 0x00;

    const auto response =
        ouch::OuchResponseDispatcher::dispatch(
            data.data(),
            data.size());

    ASSERT_TRUE(response.has_value());

    ASSERT_TRUE(
        std::holds_alternative<ouch::Executed>(
            *response));

    const auto& executed =
        std::get<ouch::Executed>(
            *response);

    EXPECT_EQ(executed.userRefNum, 1U);
    EXPECT_EQ(executed.quantity, 25U);
    EXPECT_EQ(executed.price, 12345U);
    EXPECT_EQ(executed.matchNumber, 99U);
}
