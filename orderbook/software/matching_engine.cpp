#include "array_order_book.hpp"

int main()
{
    ArrayOrderBook book;

    Order o1{
        .id = 1,
        .side = Side::Buy,
        .price = 100,
        .quantity = 10
    };

    Order o2{
        .id = 2,
        .side = Side::Buy,
        .price = 100,
        .quantity = 20
    };

    book.addOrder(&o1);
    book.addOrder(&o2);

    book.cancelOrder(1);

    return 0;
}
