#include <gtest/gtest.h>

#include "execution/ouch/ouch_decoder.hpp"

#include <array>
#include <cstdint>

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

