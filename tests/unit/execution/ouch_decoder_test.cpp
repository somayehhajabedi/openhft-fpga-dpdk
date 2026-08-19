#include <gtest/gtest.h>

#include "execution/ouch/ouch_decoder.hpp"
#include "execution/ouch/ouch_encoder.hpp"
#include "execution/ouch/accepted_encoder.hpp"
#include "execution/ouch/executed_encoder.hpp"
#include "execution/ouch/ouch_response_dispatcher.hpp"

#include <array>
#include <cstdint>
#include <variant>

TEST(
    OuchDecoderTest,
    DecodesAcceptedMessage)
{
    std::array<std::uint8_t, 64> data{};

    data[0] = 'A';

    // Timestamp = 1
    data[8] = 0x01;

    // UserRefNum = 0x01020304
    data[9]  = 0x01;
    data[10] = 0x02;
    data[11] = 0x03;
    data[12] = 0x04;

    data[13] = 'B';

    // Quantity = 100
    data[14] = 0x00;
    data[15] = 0x00;
    data[16] = 0x00;
    data[17] = 0x64;

    data[18] = 'A';
    data[19] = 'A';
    data[20] = 'P';
    data[21] = 'L';
    data[22] = ' ';
    data[23] = ' ';
    data[24] = ' ';
    data[25] = ' ';

    // Price = 12345 = 0x3039


    data[24 + 7] = 0x30;
    data[24 + 8] = 0x39;

    // Correct price bytes at offsets 18+8 = 26..33
    data[26] = 0x00;
    data[27] = 0x00;
    data[28] = 0x00;
    data[29] = 0x00;
    data[30] = 0x00;
    data[31] = 0x00;
    data[32] = 0x30;
    data[33] = 0x39;

    data[34] = '0';
    data[35] = 'Y';

    // Order reference number = 99
    data[36] = 0x00;
    data[37] = 0x00;
    data[38] = 0x00;
    data[39] = 0x00;
    data[40] = 0x00;
    data[41] = 0x00;
    data[42] = 0x00;
    data[43] = 0x63;

    data[44] = 'A';
    data[45] = 'N';
    data[46] = 'N';
    data[47] = 'L';

    const std::array<char, 14> clOrdId{
        'C', 'L', 'O', 'R', 'D',
        '0', '0', '0', '0', '0',
        '0', '0', '0', '1'
    };

    for (std::size_t i = 0;
         i < clOrdId.size();
         ++i)
    {
        data[48 + i] =
            static_cast<std::uint8_t>(
                clOrdId[i]);
    }

    data[62] = 0x00;
    data[63] = 0x00;

    const auto accepted =
        ouch::OuchDecoder::decodeAccepted(
            data.data(),
            data.size());

    ASSERT_TRUE(
        accepted.has_value());

    EXPECT_EQ(
        accepted->timestamp,
        1U);

    EXPECT_EQ(
        accepted->userRefNum,
        0x01020304U);

    EXPECT_EQ(
        accepted->side,
        Side::Buy);

    EXPECT_EQ(
        accepted->quantity,
        100U);

    EXPECT_EQ(
        accepted->symbol[0],
        'A');

    EXPECT_EQ(
        accepted->symbol[3],
        'L');

    EXPECT_EQ(
        accepted->price,
        12345U);

    EXPECT_EQ(
        accepted->orderReferenceNumber,
        99U);

    EXPECT_EQ(
        accepted->orderState,
        ouch::OrderState::Live);

    EXPECT_EQ(
        accepted->appendageLength,
        0U);
}

TEST(
    OuchDecoderTest,
    DecodesRejectedMessage)
{
    std::array<std::uint8_t, 31> data{};

    // Message Type
    data[0] = 'J';

    // Timestamp = 1
    data[8] = 0x01;

    // UserRefNum = 0x01020304
    data[9]  = 0x01;
    data[10] = 0x02;
    data[11] = 0x03;
    data[12] = 0x04;

    // Reject reason = 42
    data[13] = 0x00;
    data[14] = 0x2A;

    const std::array<char, 14> clOrdId{
        'C', 'L', 'O', 'R', 'D',
        '0', '0', '0', '0', '0',
        '0', '0', '0', '1'
    };

    for (std::size_t i = 0;
         i < clOrdId.size();
         ++i)
    {
        data[15 + i] =
            static_cast<std::uint8_t>(
                clOrdId[i]);
    }

    // No optional appendages
    data[29] = 0x00;
    data[30] = 0x00;

    const auto rejected =
        ouch::OuchDecoder::decodeRejected(
            data.data(),
            data.size());

    ASSERT_TRUE(
        rejected.has_value());

    EXPECT_EQ(
        rejected->timestamp,
        1U);

    EXPECT_EQ(
        rejected->userRefNum,
        0x01020304U);

    EXPECT_EQ(
        static_cast<std::uint16_t>(
            rejected->reason),
        42U);

    EXPECT_EQ(
        rejected->clOrdId[0],
        'C');

    EXPECT_EQ(
        rejected->clOrdId[13],
        '1');

    EXPECT_EQ(
        rejected->appendageLength,
        0U);
}

TEST(
    OuchDecoderTest,
    DecodesExecutedMessage)
{
    std::array<std::uint8_t, 36> data{};

    data[0] = 'E';

    // Timestamp = 1
    data[8] = 0x01;

    // UserRefNum = 0x01020304
    data[9]  = 0x01;
    data[10] = 0x02;
    data[11] = 0x03;
    data[12] = 0x04;

    // Executed quantity = 25
    data[13] = 0x00;
    data[14] = 0x00;
    data[15] = 0x00;
    data[16] = 0x19;

    // Execution price = 12345
    data[17] = 0x00;
    data[18] = 0x00;
    data[19] = 0x00;
    data[20] = 0x00;
    data[21] = 0x00;
    data[22] = 0x00;
    data[23] = 0x30;
    data[24] = 0x39;

    // Liquidity flag
    data[25] = 'A';

    // Match number = 99
    data[26] = 0x00;
    data[27] = 0x00;
    data[28] = 0x00;
    data[29] = 0x00;
    data[30] = 0x00;
    data[31] = 0x00;
    data[32] = 0x00;
    data[33] = 0x63;

    // No appendages
    data[34] = 0x00;
    data[35] = 0x00;

    const auto executed =
        ouch::OuchDecoder::decodeExecuted(
            data.data(),
            data.size());

    ASSERT_TRUE(executed.has_value());

    EXPECT_EQ(executed->timestamp, 1U);
    EXPECT_EQ(executed->userRefNum, 0x01020304U);
    EXPECT_EQ(executed->quantity, 25U);
    EXPECT_EQ(executed->price, 12345U);
    EXPECT_EQ(executed->liquidityFlag, 'A');
    EXPECT_EQ(executed->matchNumber, 99U);
    EXPECT_EQ(executed->appendageLength, 0U);

}

TEST(
    OuchDecoderTest,
    DecodesCanceledMessage)
{
    std::array<std::uint8_t, 18> data{};

    data[0] = 'C';

    // Timestamp = 1
    data[8] = 0x01;

    // UserRefNum = 0x01020304
    data[9]  = 0x01;
    data[10] = 0x02;
    data[11] = 0x03;
    data[12] = 0x04;

    // Canceled quantity = 25
    data[13] = 0x00;
    data[14] = 0x00;
    data[15] = 0x00;
    data[16] = 0x19;

    // Reason
    data[17] = 'U';

    const auto canceled =
        ouch::OuchDecoder::decodeCanceled(
            data.data(),
            data.size());

    ASSERT_TRUE(
        canceled.has_value());

    EXPECT_EQ(
        canceled->timestamp,
        1U);

    EXPECT_EQ(
        canceled->userRefNum,
        0x01020304U);

    EXPECT_EQ(
        canceled->quantity,
        25U);

    EXPECT_EQ(
        canceled->reason,
        'U');
}
TEST(
    OuchDecoderTest,
    DecodesEnterOrderProducedByEncoder)
{
    ouch::EnterOrder order{};

    order.userRefNum = 1234;
    order.side = Side::Buy;
    order.quantity = 50;

    order.symbol = {
        'A', 'A', 'P', 'L',
        ' ', ' ', ' ', ' '
    };

    order.price = 99;

    order.timeInForce =
        ouch::TimeInForce::Day;

    order.display =
        ouch::Display::Visible;

    order.capacity =
        ouch::Capacity::Agency;

    order.isoEligibility =
        ouch::IsoEligibility::NotEligible;

    order.crossType =
        ouch::CrossType::ContinuousMarket;

    order.appendageLength = 0;

    const auto buffer =
        ouch::OuchEncoder::encode(order);

    const auto decoded =
        ouch::OuchDecoder::decodeEnterOrder(
            buffer.data(),
            buffer.size());

    ASSERT_TRUE(
        decoded.has_value());

    EXPECT_EQ(
        decoded->userRefNum,
        order.userRefNum);

    EXPECT_EQ(
        decoded->side,
        order.side);

    EXPECT_EQ(
        decoded->quantity,
        order.quantity);

    EXPECT_EQ(
        decoded->symbol,
        order.symbol);

    EXPECT_EQ(
        decoded->price,
        order.price);

    EXPECT_EQ(
        decoded->timeInForce,
        order.timeInForce);

    EXPECT_EQ(
        decoded->display,
        order.display);

    EXPECT_EQ(
        decoded->capacity,
        order.capacity);

    EXPECT_EQ(
        decoded->isoEligibility,
        order.isoEligibility);

    EXPECT_EQ(
        decoded->crossType,
        order.crossType);

    EXPECT_EQ(
        decoded->appendageLength,
        0);
}

TEST(
    OuchDecoderTest,
    DecodesAcceptedProducedByEncoder)
{
    ouch::Accepted accepted{};

    accepted.timestamp = 123456789;
    accepted.userRefNum = 42;
    accepted.side = Side::Buy;
    accepted.quantity = 100;

    accepted.symbol = {
        'A', 'A', 'P', 'L',
        ' ', ' ', ' ', ' '
    };

    accepted.price = 12500;

    accepted.timeInForce =
        ouch::TimeInForce::Day;

    accepted.display =
        ouch::Display::Visible;

    accepted.orderReferenceNumber = 7001;

    accepted.capacity =
        ouch::Capacity::Agency;

    accepted.isoEligibility =
        ouch::IsoEligibility::NotEligible;

    accepted.crossType =
        ouch::CrossType::ContinuousMarket;

    accepted.orderState =
        ouch::OrderState::Live;

    accepted.clOrdId = {
        'O', 'R', 'D', 'E', 'R', '0', '0',
        '0', '0', '0', '0', '0', '0', '1'
    };

    accepted.appendageLength = 0;

    const auto buffer =
        ouch::AcceptedEncoder::encode(
            accepted);

    ASSERT_EQ(
        buffer.size(),
        64U);

    ASSERT_EQ(
        buffer[0],
        static_cast<std::uint8_t>('A'));

    const auto decoded =
        ouch::OuchDecoder::decodeAccepted(
            buffer.data(),
            buffer.size());

    ASSERT_TRUE(
        decoded.has_value());

    EXPECT_EQ(
        decoded->timestamp,
        accepted.timestamp);

    EXPECT_EQ(
        decoded->userRefNum,
        accepted.userRefNum);

    EXPECT_EQ(
        decoded->side,
        accepted.side);

    EXPECT_EQ(
        decoded->quantity,
        accepted.quantity);

    EXPECT_EQ(
        decoded->symbol,
        accepted.symbol);

    EXPECT_EQ(
        decoded->price,
        accepted.price);

    EXPECT_EQ(
        decoded->timeInForce,
        accepted.timeInForce);

    EXPECT_EQ(
        decoded->display,
        accepted.display);

    EXPECT_EQ(
        decoded->orderReferenceNumber,
        accepted.orderReferenceNumber);

    EXPECT_EQ(
        decoded->capacity,
        accepted.capacity);

    EXPECT_EQ(
        decoded->isoEligibility,
        accepted.isoEligibility);

    EXPECT_EQ(
        decoded->crossType,
        accepted.crossType);

    EXPECT_EQ(
        decoded->orderState,
        accepted.orderState);

    EXPECT_EQ(
        decoded->clOrdId,
        accepted.clOrdId);

    EXPECT_EQ(
        decoded->appendageLength,
        accepted.appendageLength);
}
TEST(
    OuchDecoderTest,
    DecodesExecutedProducedByEncoder)
{
    ouch::Executed executed{};

    executed.timestamp = 123456789;
    executed.userRefNum = 202;
    executed.quantity = 30;
    executed.price = 100;
    executed.liquidityFlag = 'R';
    executed.matchNumber = 9001;
    executed.appendageLength = 0;

    const auto buffer =
        ouch::ExecutedEncoder::encode(
            executed);

    ASSERT_EQ(
        buffer.size(),
        36U);

    ASSERT_EQ(
        buffer[0],
        static_cast<std::uint8_t>('E'));

    const auto response =
        ouch::OuchResponseDispatcher::dispatch(
            buffer.data(),
            buffer.size());

    ASSERT_TRUE(
        response.has_value());

    ASSERT_TRUE(
        std::holds_alternative<ouch::Executed>(
            *response));

    const auto& decoded =
        std::get<ouch::Executed>(
            *response);

    EXPECT_EQ(
        decoded.timestamp,
        executed.timestamp);

    EXPECT_EQ(
        decoded.userRefNum,
        executed.userRefNum);

    EXPECT_EQ(
        decoded.quantity,
        executed.quantity);

    EXPECT_EQ(
        decoded.price,
        executed.price);

    EXPECT_EQ(
        decoded.liquidityFlag,
        executed.liquidityFlag);

    EXPECT_EQ(
        decoded.matchNumber,
        executed.matchNumber);

    EXPECT_EQ(
        decoded.appendageLength,
        0U);
}
