
Ethernet Parser:

The Ethernet parser is a zero-copy view over the packet buffer. 
It first validates that the input pointer is non-null and that enough bytes exist for an Ethernet header. 
It then reinterprets the beginning of the packet as an EthernetHeader.
The EtherType accessor converts the 16-bit field from network byte order to host byte order using ntohs,
and the payload accessor advances the byte pointer by the Ethernet-header size.
The implementation performs no allocation or copying, but it currently assumes
a standard untagged Ethernet II frame, so VLAN support would require handling a variable payload offset.