TEST(OrderCancelMapperTest, MapsFieldsCorrectly)
{
    // Arrange
    OrderCancelWireMessage message{};

    // TODO:
    // مقداردهی order_reference_number و cancelled_shares
    // به صورت Big Endian

    // Act
    auto result =
        OrderCancelMapper::fromWire(&message);

    // Assert
    EXPECT_EQ(result.orderReferenceNumber, expectedOrderId);
    EXPECT_EQ(result.cancelledShares, expectedCancelledShares);
}