class OrderBook
{
public:

    void addOrder(Order* order);

    void cancelOrder(OrderId id);

    void modifyOrder(OrderId id, Qty newQty);

private:

};
