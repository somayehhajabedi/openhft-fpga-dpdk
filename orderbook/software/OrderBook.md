I used an intrusive doubly linked list because each order already needs stable identity and direct removal. Storing the links inside the order avoids an additional list-node allocation and lets cancel and delete operations unlink an order in constant time once it is found by ID.


