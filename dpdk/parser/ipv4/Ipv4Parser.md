The IPv4 parser is a zero-copy parser over the Ethernet payload. It first validates 
that the fixed 20-byte header is available, then interprets the bytes as a packed IPv4 header. 
It verifies version 4, extracts the IHL from the low four bits of the first byte,
converts that value from 32-bit words to bytes, and confirms the complete variable-length header is present.
The payload pointer advances by the actual IHL, so IPv4 options are supported structurally.
The total-length field is converted from network byte order and used to calculate the payload length.
One improvement I identified is validating that the declared total length does not exceed the actual 
available buffer before forwarding the payload downstream.
so, IPv4Parser validates the fixed header, version and variable IHL,
then exposes a zero-copy view of the transport payload.