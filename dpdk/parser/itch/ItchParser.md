The current ITCH parser is intentionally a lightweight, zero-copy dispatch layer 
rather than a complete message decoder. It validates that at least the common
one-byte message type is available, interprets that byte as an ITCH header,
and exposes the remaining bytes as the message body. Message-specific structures
such as Add Order, Cancel, Execute or Replace should perform their own length validation
and field decoding. One issue I identified is that messageType() currently assumes
 a non-null pointer, while the other accessors are null-safe,
so I would make that contract consistent.
So, This class is a lightweight ITCH message-type dispatcher,
not yet a complete ITCH message decoder.

The Add Order path is split into a packed wire representation, a parser and a mapper. The packed structure mirrors the 36-byte ITCH message exactly and is protected by a compile-time size assertion. The parser performs bounds checking and exposes endian-aware accessors for the protocol fields. The mapper then converts the wire message into a host-order domain model used by the order book, keeping networking details out of the matching logic. The path is zero-copy up to the mapping step, allocation-free, and uses integer quantities and prices. Improvements I identified include validating the 'A' message type, rejecting invalid side indicators, handling null mapping explicitly, and decoding the 48-bit timestamp.
So, AddOrderParser decodes the packed network representation,
while AddOrderMapper converts it into a clean host-order domain object.


