The UDP parser is a zero-copy view over the IPv4 payload. It first verifies that
at least the fixed eight-byte UDP header is available, then interprets the buffer
as a packed UDP header. Source port, destination port and datagram length are
converted from network byte order. The payload begins immediately after the fixed header,
and its length is the declared UDP length minus eight bytes. One improvement 
I identified is validating the declared UDP length against the actual number of 
available bytes before forwarding the payload to the ITCH parser.
So, UDPParser provides a zero-copy, allocation-free view of a fixed-size
eight-byte UDP header and exposes the transport payload.